// The mjpeg url.
import './styles.css';
const url = "/video";

const TYPE_JPEG = 'image/jpeg';

// const socket = new WebSocket(`ws://${location.host}/ws`);
// socket.onopen = () => {
//     socket.send('f');
// };
// socket.onerror = () => console.log('Error');
// socket.onclose = () => console.log('Closed!');

let lastTime = 0;

// socket.onmessage = (e) => {
//     if (lastTime) {
//         console.log("Frame time:", new Date().getTime() - lastTime);
//     }
//     document.getElementById('video').src = URL.createObjectURL(e.data, { type: TYPE_JPEG });
//     lastTime = new Date().getTime();
// }

//window.socket = socket;

let testBitmap;

let screenRect;
const canvas = document.getElementById('camera_canvas');
const ctx = canvas.getContext('2d');

function resizeCanvas() {
    screenRect = canvas.getBoundingClientRect();
    canvas.width = screenRect.width;
    canvas.height = screenRect.height;
};

let coord = {
    x: 0,
    y: 0
}

let zoom = 0.8;

function render() {
    if (testBitmap) {
        const posX = (canvas.width - testBitmap.width * zoom) / 2;
        const posY = (canvas.height - testBitmap.height * zoom) / 2;
        ctx.drawImage(testBitmap, posX, posY, testBitmap.width * zoom, testBitmap.height * zoom);
    }
}

resizeCanvas();
render();

window.onresize = function() {
    resizeCanvas();
}

canvas.onmousemove = function(e) {
    coord.x = e.clientX;
    coord.y = e.clientY;
}

function frame() {
    render();
    requestAnimationFrame(frame);
}

requestAnimationFrame(frame);

// const img = new Image();
// img.src = 'test.jpg';
// img.addEventListener('load', function() {
//     ctx.drawImage(img, 10, 0);
//     console.log('ss');
// })
// console.log(img);

fetch('test.jpg')
    .then(function(res) {
        return res.blob();
    })
    .then(function(blob) {
        return createImageBitmap(blob); 
    })
    .then(function(bitmap) {
        testBitmap = bitmap;
    });


document.getElementById('showOptionsBtn').addEventListener('click', function() {
    document.getElementsByClassName('options-panel')[0].classList.toggle('active');
})