// demo.js

// khi emscripten tạo ra Module, bạn sẽ có biến global `Module`
// sau đây ta wrap lại các hàm để gọi dễ dàng:

const wasm = {};

let isRandomGame = false; // global variable to track if the game is a random game

function initWasm() {
  wasm.newDefault  = Module.cwrap('new_default',  'number',  []);
   wasm.playMove    = Module.cwrap('play_move',    'void',    ['number','number','number','number']);
  wasm.shuffle     = Module.cwrap('restart',      'void',    ['number']);
  wasm.undo        = Module.cwrap('undo',         'void',    ['number']);
  wasm.redo        = Module.cwrap('redo',         'void',    ['number']);
  wasm.solve       = Module.cwrap('solve',        'number',  ['number']);
  wasm.getShape    = Module.cwrap('get_piece_shape','number',['number','number','number']);
  wasm.getOrient   = Module.cwrap('get_piece_orientation','number',['number','number','number']);
  wasm.nbRows      = Module.cwrap('nb_rows',      'number',  ['number']);
  wasm.nbCols      = Module.cwrap('nb_cols',      'number',  ['number']);
  wasm.isGameWon   = Module.cwrap('won',  'number',  ['number']);
  wasm.newRandom  = Module.cwrap('new_random', 'number', ['number', 'number', 'number', 'number', 'number']);
  wasm.save       = Module.cwrap('save', 'void', ['number', 'string']);
  wasm.load        = Module.cwrap('load', 'number', ['string']);


}

// load hình ảnh sprite cho các piece (giống như trong game_sdl)
const IMG_BASE = './res/';
const pieceTextures = {};
let backgroundImage;

// utility để load ảnh
function loadImage(src) {
  return new Promise((res,rej) => {
    const img = new Image();
    img.onload  = ()=>res(img);
    img.onerror = ()=>res(null);
    img.src = src;
  });
}

async function loadTextures() {
  // background
  backgroundImage = await loadImage(IMG_BASE + 'BACKGROUND.jpg');

  // định nghĩa đường dẫn
  const defs = [
    { shape:0, normal:['SHAPE/EMPTY.png'],           dark:['DARKSHAPE/D_EMPTY.png'] },
    { shape:1, normal:['SHAPE/NN_SHAPE.png','SHAPE/NE_SHAPE.png','SHAPE/NS_SHAPE.png','SHAPE/NW_SHAPE.png'],
               dark:['DARKSHAPE/D_NN_SHAPE.png','DARKSHAPE/D_NE_SHAPE.png','DARKSHAPE/D_NS_SHAPE.png','DARKSHAPE/D_NW_SHAPE.png'] },
    { shape:2, normal:['SHAPE/SN_SHAPE.png','SHAPE/SE_SHAPE.png'], // segment 2 hướng
               dark:['DARKSHAPE/D_SN_SHAPE.png','DARKSHAPE/D_SE_SHAPE.png'] },
    { shape:3, normal:['SHAPE/CN_SHAPE.png','SHAPE/CE_SHAPE.png','SHAPE/CS_SHAPE.png','SHAPE/CW_SHAPE.png'],
               dark:['DARKSHAPE/D_CN_SHAPE.png','DARKSHAPE/D_CE_SHAPE.png','DARKSHAPE/D_CS_SHAPE.png','DARKSHAPE/D_CW_SHAPE.png'] },
    { shape:4, normal:['SHAPE/TN_SHAPE.png','SHAPE/TE_SHAPE.png','SHAPE/TS_SHAPE.png','SHAPE/TW_SHAPE.PNG'],
               dark:['DARKSHAPE/D_TN_SHAPE.png','DARKSHAPE/D_TE_SHAPE.png','DARKSHAPE/D_TS_SHAPE.png','DARKSHAPE/D_TW_SHAPE.png'] },
    { shape:5, normal:['SHAPE/ZCROSS_SHAPE.png'], dark:['DARKSHAPE/zD_CROSS_SHAPE.png'] }
  ];

  for (const def of defs) {
    const norm = await Promise.all(def.normal.map(f=>loadImage(IMG_BASE+f)));
    const dark = await Promise.all(def.dark.map(f=>loadImage(IMG_BASE+f)));
    pieceTextures[def.shape] = { normal:norm, dark:dark };
  }
}

// vẽ một piece tại col,row
function drawPiece(shape, orient, col, row, size, selected) {
  const set = pieceTextures[shape];
  if (!set) return;
  const idx = orient % set.normal.length;
  const img = selected ? set.dark[idx] : set.normal[idx];
  ctx.drawImage(img, col*size, row*size, size, size);
}

// ----- setup canvas và logic -----
let canvas, ctx;
let gamePtr,gamePtr_tmp, CELL_SIZE;
let selected = { x:-1, y:-1 };
const BUTTONS = { shuffle:null, solve:null, undo:null, redo:null, help:null };

function init() {
    canvas = document.getElementById('mycanvas');
    ctx = canvas.getContext('2d');

    // Khởi tạo WASM
    initWasm();

    // Khởi tạo game
    gamePtr = wasm.newDefault();
    isRandomGame = false;

    // Load textures
    loadTextures().then(() => {
        const R = wasm.nbRows(gamePtr), C = wasm.nbCols(gamePtr);
        const maxW = window.innerWidth * 0.7, maxH = window.innerHeight * 0.7;
        CELL_SIZE = Math.min(Math.floor(maxW / C), Math.floor(maxH / R));
        canvas.width = C * CELL_SIZE;
        canvas.height = R * CELL_SIZE + 50; // +50px cho vùng button

        // Định nghĩa vị trí các nút
        BUTTONS.shuffle = { x: 10, y: R * CELL_SIZE + 10, w: 80, h: 30 };
        BUTTONS.solve = { x: 100, y: R * CELL_SIZE + 10, w: 80, h: 30 };
        BUTTONS.undo = { x: 190, y: R * CELL_SIZE + 10, w: 40, h: 30 };
        BUTTONS.redo = { x: 240, y: R * CELL_SIZE + 10, w: 40, h: 30 };
        BUTTONS.help = { x: 290, y: R * CELL_SIZE + 10, w: 40, h: 30 };
        BUTTONS.random_game = { x: 340, y: R * CELL_SIZE + 10, w: 120, h: 30 };

        // Gắn sự kiện
        canvas.addEventListener('click', onCanvasClick);
        canvas.addEventListener('click', onButtonClick);
        window.addEventListener('keydown', onKeyDown);

        // Gắn sự kiện cho modal random game
        document.getElementById('randomGameForm').onsubmit = (ev) => {
            ev.preventDefault();
            const nbRows = parseInt(document.getElementById('nbRows').value, 10);
            const nbCols = parseInt(document.getElementById('nbCols').value, 10);
            const wrapping = parseInt(document.getElementById('wrapping').value, 10);
            const nbEmpty = parseInt(document.getElementById('nbEmpty').value, 10);
            const nbExtra = parseInt(document.getElementById('nbExtra').value, 10);

            gamePtr = wasm.newRandom(nbRows, nbCols, wrapping, nbEmpty, nbExtra);
            isRandomGame = true; 
            selected = { x: -1, y: -1 };
            document.getElementById('randomGameModal').style.display = 'none';

            const R = wasm.nbRows(gamePtr), C = wasm.nbCols(gamePtr);
            CELL_SIZE = Math.min(Math.floor(window.innerWidth * 0.7 / C), Math.floor(window.innerHeight * 0.7 / R));
            canvas.width = C * CELL_SIZE;
            canvas.height = R * CELL_SIZE + 50;
            wasm.shuffle(gamePtr); // Shuffle the game after creating a new random game
            render();
        };

        document.getElementById('closeRandomGameModal').onclick = () => {
            document.getElementById('randomGameModal').style.display = 'none';
        };
        document.getElementById('closeHelpModal').onclick = () => {
          document.getElementById('helpModal').style.display = 'none'; // Hide the help modal
        };
        
        requestAnimationFrame(render);
    });
}
function saveGameToFile() {
    const filename = "savegame.txt"; // Default filename
    const saveData = Module.FS_readFile('/tmp/savegame.txt'); // Read the file from Emscripten's virtual file system

    // Create a Blob and trigger download
    const blob = new Blob([saveData], { type: 'text/plain' });
    const link = document.createElement('a');
    link.href = URL.createObjectURL(blob);
    link.download = filename;
    link.click();

    console.log("Game saved to file:", filename);
}

function loadGameFromFile() {
    const fileInput = document.createElement('input');
    fileInput.type = 'file';
    fileInput.accept = '.txt'; // Restrict to text files

    fileInput.addEventListener('change', (event) => {
        const file = event.target.files[0];
        if (!file) {
            console.error("No file selected.");
            return;
        }

        const reader = new FileReader();
        reader.onload = (e) => {
            const fileContent = e.target.result;

            // Write the file content to Emscripten's virtual file system
            const filename = 'savegame.txt';
            // Module.FS_writeFile(filename, new Uint8Array(fileContent));

            // Load the game using the wasm.load function
            const newGamePtr = wasm.load(filename);
            if (newGamePtr) {
                gamePtr = newGamePtr;
                selected = { x: -1, y: -1 };
                console.log("Game loaded successfully!");
                render();
            } else {
                console.error("Failed to load game.");
            }
        };

        reader.onerror = (e) => {
            console.error("Error reading file:", e);
        };

        reader.readAsArrayBuffer(file);
    });

    fileInput.click(); // Trigger the file input dialog
}



// vẽ game
function render() {
  const R = wasm.nbRows(gamePtr), C = wasm.nbCols(gamePtr);
  // clear
  if (backgroundImage) {
    ctx.drawImage(backgroundImage, 0,0, canvas.width, canvas.height);
  } else {
    ctx.fillStyle='#333'; ctx.fillRect(0,0,canvas.width,canvas.height);
  }

  // draw grid pieces
  for (let i=0;i<R;i++) for (let j=0;j<C;j++) {
    const s = wasm.getShape(gamePtr,i,j);
    const o = wasm.getOrient(gamePtr,i,j);
    const sel = (selected.x===j && selected.y===i);
    drawPiece(s,o,j,i,CELL_SIZE,sel);
  }

  // highlight selected
  if (selected.x>=0) {
    ctx.strokeStyle='yellow';
    ctx.lineWidth=3;
    ctx.strokeRect(selected.x*CELL_SIZE, selected.y*CELL_SIZE, CELL_SIZE, CELL_SIZE);
  }

  // draw buttons
  ctx.font='16px sans-serif';
  ctx.textBaseline='middle';
  for (let [name, r] of Object.entries(BUTTONS)) {
    ctx.fillStyle='#555';
    ctx.fillRect(r.x,r.y,r.w,r.h);
    ctx.fillStyle='white';
    ctx.fillText(name, r.x+5, r.y+r.h/2);
  }

  requestAnimationFrame(render);
}

// click grid: chọn ô
function onCanvasClick(ev) {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    const mx = (ev.clientX - rect.left) * scaleX;
    const my = (ev.clientY - rect.top) * scaleY;
  // nếu click trong vùng button thì bỏ qua
    if (my > wasm.nbRows(gamePtr)*CELL_SIZE) return;
    const j = Math.floor(mx/CELL_SIZE), i = Math.floor(my/CELL_SIZE);
    if (i>=0 && i<wasm.nbRows(gamePtr) && j>=0 && j<wasm.nbCols(gamePtr)) {
        selected = { x:j, y:i };
    } else selected={x:-1,y:-1};
    checkGameStatus();
}

//CHeck if game is won
function checkGameStatus() {
  console.log("Checking game status...");
  if (wasm.isGameWon(gamePtr)) {
    console.log("Game won!");
    const modal = document.getElementById('winModal');
    modal.style.display = 'flex'; // Show modal

    // // Handle "Restart Game" button
    // document.getElementById('restartGame').onclick = () => {
    //   modal.style.display = 'none'; // Hide modal
    //   gamePtr = wasm.shuffle(); // Reset game
    //   gamePtr = wasm.shuffle(); // Reset game
    //   selected = { x: -1, y: -1 }; // Reset selection
    // };

    // Handle "Close" button
    document.getElementById('closeModal').onclick = () => {
      modal.style.display = 'none'; // Hide modal
    };

    // Handle "Play Again" button
    document.getElementById('playAgain').onclick = () => {
      modal.style.display = 'none'; // Hide modal
      gamePtr = wasm.newDefault(); // Reset game
      selected = { x: -1, y: -1 }; // Reset selection
    };
  } else {
    console.log("Game not won yet.");
  }
}

// click button
function onButtonClick(ev) {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    const mx = (ev.clientX - rect.left) * scaleX;
    const my = (ev.clientY - rect.top) * scaleY;
    console.log("canvas size:", canvas.width, canvas.height);
    console.log("display size:", rect.width, rect.height);
    console.log('Mouse click:', mx, my);
    for (let [name,r] of Object.entries(BUTTONS)) {
        if (mx>=r.x && mx<=r.x+r.w && my>=r.y && my<=r.y+r.h) {
            console.log('Button:', name);
        switch(name) {
            case 'shuffle': wasm.shuffle(gamePtr); break;
            case 'solve':   wasm.solve(gamePtr); setTimeout(() => checkGameStatus(), 100); // Delay to ensure state is updated  break;
            case 'undo':    wasm.undo(gamePtr);   break;
            case 'redo':    wasm.redo(gamePtr);   break;
            case 'help':     const helpModal = document.getElementById('helpModal');
            helpModal.style.display = 'flex'; // Show the help modal
            break;
            case 'random_game':
                  const randomGameModal = document.getElementById('randomGameModal');
                  randomGameModal.style.display = 'flex'; // Show the modal
                  break;
            break;
        }
        
        break;
        }
    }
}

// phím c/a để xoay
function onKeyDown(ev) {
  if (selected.x<0) return;
  if (ev.key==='c' || ev.key==='ArrowRight') wasm.playMove(gamePtr, selected.y, selected.x, 1);
  if (ev.key==='a'|| ev.key==='ArrowLeft') wasm.playMove(gamePtr, selected.y, selected.x, -1);
  if (ev.key==='s') wasm.save(gamePtr, 'savegame.txt');
  if (ev.key==='l') loadGameFromFile();
  checkGameStatus();
}

// start khi Module sẵn sàng
if (Module['calledRun']) init();
else Module['onRuntimeInitialized']=init;
