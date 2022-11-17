$(() => {
    let count = 0

    let wes = new WSConnection()
    let summary = new Summary()

    function array2hex(buf) {
        return [...new Uint8Array(buf)]
            .map(b => b.toString(16).padStart(2, '0'))
            .join(' ')
    }
    wes.onmessage = (data) => {
        count++;
        const littleEndian = true
        // console.log("ws message " + data)
        const view = new DataView(data)
        // const x = array2hex(new ArrayBuffer(data, 2))
        // const y = data.slice(2)
        const fd = view.getUint16(0, littleEndian)
        $('#frame').val(String(fd).padStart(4, '0') + ' ' + array2hex(data.slice(2)))
        summary.addId(fd)
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
})

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
    xsend(frame) {
        this.wes.send(frame)
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
    addId(id) {
        if ( ! this.ids.includes(id)) {
            this.ids.push(id)
            this.ids.sort((a, b) => { return a - b })
            $(this).trigger('update')
        }
    }
}
