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
        const statusFrameId = 0x3fe

        this.statusPanel = new StatusPanel()
        let totalFrames = 0

        let webSocket = new WSConnection()
        let frameSummaries = {}
    
        webSocket.onmessage = (data) => {
            totalFrames++;

            const frame = new Frame(data)

            this.dispatchFrame(frame)
            if (frame.fd === statusFrameId) {
                this.statusPanel.update(new Uint8Array(frame.data))
            }
            // find or create frame summary
            const fd = frame.fd
            let frameSummary = frameSummaries[fd]
            if (frameSummary === undefined) {
                frameSummary = frameSummaries[fd] = new FrameSummary(fd)
                this.createFrameView(frameSummary)
            }
            frameSummary.event(frame)   
        }
        $(webSocket).bind('status', (event, status) => {
            const statusString = webSocket.statusString(status)
            $('#websocketstatus').val(statusString)
        })

        
        // a bit tricky to get realtime changes
        $('#rpm').mousemove(event => {
            const rpm = document.getElementById('rpm').value
            console.log(rpm)
            // wes.send(rpm)
        })
        // fixme separate button send codes
        $('#send').click(event => {
            webSocket.send(200)
        })
        $('#fuzzing').click(event => {
            webSocket.send(200)
        })
        $(this.summary).on('update', event => {
            const list = this.summary.ids.join(',')
            $('#summary').val(list)
        })
    
        const sample = 1000 // ms
        setInterval(() => {
            const sampledFrameRate = totalFrames / (sample / 1000)
            totalFrames = 0
            $('#rate').val(sampledFrameRate)
        }, sample)
    
    }
    createFrameView(frameSummary) {
        const tb = $('table#frames tbody')
        const tr = $('<tr>').appendTo(tb)

        const count = $('<td>').appendTo(tr).addClass('count')
        const period = $('<td>').appendTo(tr).addClass('period')
        $('<td>').appendTo(tr).text(frameSummary.fd.toString(16)).addClass('code')
        const flags = $('<td>').appendTo(tr).addClass('flags')
        const payload = $('<td>').appendTo(tr).addClass('payload')

        $(frameSummary).bind('update', (event, framesummary, frame) => {
            const target = event.currentTarget

            count.text(framesummary.counter)
            period.text(Math.round(framesummary.period))
            flags.text(frame.flagsAsText)
            payload.text(array2hex(frame.data))
        })
    }
}

class WSConnection {
    constructor() {
        this.onMessage = () => {}
        this.url = 'ws://' + window.location.host + '/ws'
        this.reconnect()

        setInterval(() => {
            $(this).trigger('status', [ this.socket.readyState ])
        }, 1000)
    }
    init() {

    }
    reconnect() {
        try {
            this.socket = new WebSocket(this.url)
            clearInterval(this.reconnectInterval)
            this.socket.binaryType = 'arraybuffer'

            this.socket.onopen = (event) => {
                console.log('websocket opened ' + Date())
                 $(this).trigger('status', [ this.socket.readyState ])
            }
            this.socket.onclose = (event) => {
                console.log('websocket closed ' + Date())
                this.reconnectInterval = setInterval(() => {
                    this.reconnect()
                }, 1000)        
            }
            this.socket.onerror = (event) => {
                console.log('websocket error ' + Date())
            }
            this.socket.onmessage = (event) => {
                this.onmessage(event.data)
            }
        }
        catch (err) {
            console.log('websocket connect error: ' + err)
        }
    }
    send(rpm) {
        const frame = new ArrayBuffer(10)
        const view = new DataView(frame)
        view.setUint16(0, 0x320, true)
        view.setUint16(3, 2000 * 100)
        this.socket.send(frame)
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
    // ByteArray
    constructor(bytearray) {
        const littleEndian = true
        const view = new DataView(bytearray)
        this.fd = view.getUint32(0, littleEndian)
        this.flags = view.getUint8(4)
        this.data = bytearray.slice(5)
    }
    get flagsAsText() {
        const srrMask = 0x01
        const ideMask = 0x02
        let text = ''
        if (this.flags & srrMask) {
            text += 'R'
        }
        if (this.flags & ideMask) {
            text += 'E'
        }
        return text
    }
}
class FrameSummary {
    constructor(fd) {
        this.fd = fd
        this.sampleStartTime = Date.now()
        this.counter = 0
    }
    event(framedata) {
        this.counter += 1
        this.sampleLastTime = Date.now()
        $(this).trigger('update', [ this, framedata ])
    }
    get period() {
        return (this.sampleLastTime - this.sampleStartTime) / this.counter
    }
}
class StatusPanel {
    constructor() {
        const status = $('#status');
        status.empty();
        const t = $('<table>').appendTo(status);
        const tb = $('<tbody>').appendTo(t);

        const addRow = (label) => {
            let tr = $('<tr>').appendTo(tb)
            $('<th>').appendTo(tr).text(label.toUpperCase())
            this[label] = $('<td>').appendTo(tr);
            this[label + 'flags'] = $('<td>').appendTo(tr);
        }
        addRow('eflg')
        addRow('tec')
        addRow('canintf')
        addRow('caninte')
        addRow('tb0ctrl')
    }
    // see CanStatus in mcp25625.h
    update(status) {
        this.eflg.text(status[0].toString(16).padStart(2, '0'))
        this.eflgflags.text(new Mcp25625Flags().eflgNames(status[0]))
        this.tec.text(status[1].toString().padStart(2, '0'))
        this.canintf.text(status[2].toString(16).padStart(2, '0'))
        this.canintfflags.text(new Mcp25625Flags().caninfNames(status[2]))
        this.caninte.text(status[3].toString(16).padStart(2, '0'))
        this.tb0ctrl.text(status[4].toString(16).padStart(2, '0'))
        this.tb0ctrlflags.text(new Mcp25625Flags().tbnctrlNames(status[4]))
    }
}

class Mcp25625Flags {
    eflgNames(flags) {
        const bitNames = { RX1OVR: 7, RX0OVR: 6, TXBO: 5, TXEP: 4, RXEP: 3, TXWAR: 2, RXWAR: 1, EWARN: 0}
        let names = []
        for (const [key, value] of Object.entries(bitNames)) {
            const mask = 1 << value
            if (flags & mask) {
                names.push(key)
            }
        }
        return names.join(',')
    }
    caninfNames = (flags) => {
        const bitNames = { MERRF: 7, WAKIF: 6, ERRIF: 5, TX2IF: 4, TX1IF: 3, TX0IF: 2, RX1IF: 1, RX0IF: 0}
        let names = []
        for (const [key, value] of Object.entries(bitNames)) {
            const mask = 1 << value
            if (flags & mask) {
                names.push(key)
            }
        }
        return names.join(',')
    }
    tbnctrlNames = (flags) => {
        const bitNames = { ABTF: 6, MLOA: 5, TXERR: 4, TXREQ: 3, TXP1: 1, TXP0: 0}
        return this.mapFlags(flags, bitNames)
    }
    mapFlags = (flags, bitNames) => {
        let names = []
        for (const [key, value] of Object.entries(bitNames)) {
            const mask = 1 << value
            if (flags & mask) {
                names.push(key)
            }
        }
        return names.join(',')
    }
}