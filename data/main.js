$(() => {
    let count = 0

    let wes = new WSConnection()

    function array2hex(buf) {
        return [...new Uint8Array(buf)]
            .map(b => b.toString(16).padStart(2, '0'))
            .join(' ')
    }
    wes.onmessage = (data) => {
        count++;
        const lttleEndian = true
        // console.log("ws message " + data)
        const view = new DataView(data)
        const x = array2hex(new ArrayBuffer(data, 2))
        const y = data.slice(2)
        $('#frame').val(view.getUint16(0, lttleEndian) + ' ' + array2hex(data.slice(2)))
    }

    $('#request').click(() => {
        wes.send('request')
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
            if (this.wes.readyState == WebSocket.OPEN) {
                this.wes.send('sync')
            }
        }, 1000)
    }
    init() {

    }
    send(frame) {
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
