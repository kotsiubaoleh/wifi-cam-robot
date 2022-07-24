/*
 * ATTENTION: The "eval" devtool has been used (maybe by default in mode: "development").
 * This devtool is neither made for production nor for readable output files.
 * It uses "eval()" calls to create a separate source file in the browser devtools.
 * If you are trying to read the output file, select a different devtool (https://webpack.js.org/configuration/devtool/)
 * or disable the default devtool with "devtool: false".
 * If you are looking for production-ready output files, see mode: "production" (https://webpack.js.org/configuration/mode/).
 */
/******/ (() => { // webpackBootstrap
/******/ 	var __webpack_modules__ = ({

/***/ "./src/index.js":
/*!**********************!*\
  !*** ./src/index.js ***!
  \**********************/
/***/ (() => {

eval("// The mjpeg url.\nconst url = \"/video\";\n\nconst TYPE_JPEG = 'image/jpeg';\n\n// const socket = new WebSocket(`ws://${location.host}/ws`);\n// socket.onopen = () => {\n//     socket.send('f');\n// };\n// socket.onerror = () => console.log('Error');\n// socket.onclose = () => console.log('Closed!');\n\nlet lastTime = 0;\n\n// socket.onmessage = (e) => {\n//     if (lastTime) {\n//         console.log(\"Frame time:\", new Date().getTime() - lastTime);\n//     }\n//     document.getElementById('video').src = URL.createObjectURL(e.data, { type: TYPE_JPEG });\n//     lastTime = new Date().getTime();\n// }\n\n//window.socket = socket;\n\nconst ctx = document.getElementById('camera_canvas').getContext('2d');\nctx.fillSyle = 'black';\nctx.fillRect(0,0, 100, 100);\n\n//# sourceURL=webpack://wifi-cam-robot/./src/index.js?");

/***/ })

/******/ 	});
/************************************************************************/
/******/ 	
/******/ 	// startup
/******/ 	// Load entry module and return exports
/******/ 	// This entry module can't be inlined because the eval devtool is used.
/******/ 	var __webpack_exports__ = {};
/******/ 	__webpack_modules__["./src/index.js"]();
/******/ 	
/******/ })()
;