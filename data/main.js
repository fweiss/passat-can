$(() => {
    const app = new App()
})

function array2hex(buf) {
    return [...new Uint8Array(buf)]
        .map(b => b.toString(16).padStart(2, '0'))
        .join(' ')
}

class App {
    dispatchFrame(fd, payload) {
        // frameModel
    }
    constructor() {
        let count = 0

        let wes = new WSConnection()
        let frames = {}
    
        wes.onmessage = (data) => {
            count++;

            const littleEndian = true
            const view = new DataView(data)
            const fd = view.getUint16(0, littleEndian)
            const payload = data.slice(2)

            this.dispatchFrame(fd, payload)
    
            let frame = this.frame = frames[fd]
            if (frame === undefined) {
                frame = frames[fd] = new Frame(fd)
                this.createFrameView(frame)
            }
            frame.event(payload)
    
        }

        
        // a bit tricky to get realtime changes
        $('#rpm').mousemove(event => {
            const rpm = document.getElementById('rpm').value
            console.log(rpm)
            // wes.send(rpm)
        })
        $('#send').click(event => {
            wes.send(200)
        })
        $(this.summary).on('update', event => {
            const list = this.summary.ids.join(',')
            $('#summary').val(list)
        })
    
        const sample = 1000 // ms
        setInterval(() => {
            let rate = count / (sample / 1000)
            count = 0
            $('#rate').val(rate)
        }, sample)
    
    }
    createFrameView(frame) {
        const tb = $('table#frames tbody')
        const tr = $('<tr>').appendTo(tb)
        $('<td>').appendTo(tr).text(frame.fd).addClass('code')
        const period = $('<td>').appendTo(tr).addClass('period')
        const payload = $('<td>').appendTo(tr).addClass('payload')
        $(frame).bind('update', (event, data) => {
            const target = event.currentTarget
            period.text(Math.round(target.period))
            payload.text(array2hex(data))
        })
    }
}

class WSConnection {
    constructor() {
        const websocketstatus = $('#websocketstatus')
        this.wes = new WebSocket('ws://' + window.location.host + '/ws')
        this.wes.binaryType = 'arraybuffer'

        this.wes.onopen = (event) => {
            console.log('websocket opened')
            websocketstatus.val(this.statusString(this.wes.readyState))
        }
        this.wes.onclose = (event) => {
            console.log('websocket closed')
        }
        this.wes.onerror = (event) => {
            console.log('websocket error')
        }
        this.onMessage = () => {}
        this.wes.onmessage = (event) => {
            this.onmessage(event.data)
        }
        setInterval(() => {
            // console.log('websocket readystate: ' + this.statusString(this.wes.readyState))
            websocketstatus.val(this.statusString(this.wes.readyState))
        }, 1000)
    }
    init() {

    }
    send(rpm) {
        const frame = new ArrayBuffer(10)
        const view = new DataView(frame)
        view.setUint16(0, 0x320, true)
        view.setUint16(3, 2000 * 100)
        this.wes.send(frame)
    }
    statusString(readyState) {
        const map = {
            [WebSocket.CONNECTING]: 'connecting',
            [WebSocket.OPEN]: 'open',
            [WebSocket.CLOSING]: 'closing',
            [WebSocket.CLOSED]: 'closed',
        }
        return map[readyState] || 'unknown'
    }
}
class Frame {
    constructor(fd) {
        this.fd = fd
        this.sampleStartTime = Date.now()
        this.counter = 0
    }
    event(payload) {
        this.counter += 1
        this.sampleLastTime = Date.now()
        $(this).trigger('update', payload)
    }
    get period() {
        return (this.sampleLastTime - this.sampleStartTime) / this.counter
    }
}
