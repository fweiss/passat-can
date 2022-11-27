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
        this.trace(fd, payload)
        // summary
        // frameModel
    }
    constructor() {
        let count = 0

        let wes = new WSConnection()
        let summary = new Summary()
        let frames = {}
    
        wes.onmessage = (data) => {
            count++;

            const littleEndian = true
            const view = new DataView(data)
            const fd = view.getUint16(0, littleEndian)
            const payload = data.slice(2)

            this.dispatchFrame(fd, payload)

            summary.addId(fd)
    
            // model
            let frame = this.frame = frames[fd]
            if (frame === undefined) {
                frame = frames[fd] = new Frame(fd)
                // view
                $(frame).bind('update', e => {
                    const target = e.currentTarget
                    const fdClass = 'fd' + target.fd
                    const ol = $('#frames')
                    let li = $('#frames li.' + fdClass)
                    if (li.length === 0) {
                        console.log(target.fd)
                        li = $('<li>').appendTo(ol)
                        li.addClass(fdClass)
                        $('<span>').appendTo(li).text(target.fd)
                        $('<span>').appendTo(li)

                    }
                    $(li).children().eq(1).text(Math.round(target.period))
                })
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
        $(summary).on('update', event => {
            const list = summary.ids.join(',')
            console.log(list)
            $('#summary').val(list)
        })
    
        const sample = 1000 // ms
        setInterval(() => {
            let rate = count / (sample / 1000)
            count = 0
            $('#rate').val(rate)
        }, sample)
    
    }
    trace(fd, payload) {
        $('#frame').val(String(fd).padStart(4, '0') + ' ' + array2hex(payload))
    }
}

class WSConnection {
    constructor() {
        this.wes = new WebSocket('ws://' + window.location.host + '/ws')
        this.wes.binaryType = 'arraybuffer'

        this.wes.onopen = (event) => {
            console.log('websocket opened')
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
            console.log('websocket readystate: ' + this.statusString(this.wes.readyState))
            // FIXME let the server manage the ping
            // if (this.wes.readyState == WebSocket.OPEN) {
            //     this.wes.send('sync')
            // }
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

class Summary {
    constructor() {
        this.ids = []
    }
    addId(fd) {
        if ( ! this.ids.includes(fd)) {
            this.ids.push(fd)
            this.ids.sort((a, b) => { return a - b })
            $(this).trigger('update', fd)
        }
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
        $(this).trigger('update')
    }
    get period() {
        return (this.sampleLastTime - this.sampleStartTime) / this.counter
    }
}
