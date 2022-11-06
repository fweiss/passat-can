$(() => {
    console.log('hello')

    let wes = new WebSocket('ws://' + window.location.host + '/ws')
    wes.onopen = (event) => {
        console.log('websocket opened')
    }
    wes.onclose = (event) => {
        console.log('websocket closed')
    }
    wes.onerror = (event) => {
        console.log('websocket error')
    }
    wes.onmessage = (event) => {
        console.log("ws message " + event.data)
        $('#frame').val(event.data)
    }
    // window.setTimeout(() => { wes.send('hello websocket') }, 5000)

    $('#request').click(() => {
        wes.send('request')
    })
})
