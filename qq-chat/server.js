const HOST = '192.168.124.129';

const express = require('express');
const app = express();
const WebSocket = require('ws');
const { ObjectId } = require('mongodb');
const { connectToDb, saveText, saveFile, closeConnection, getUnsentMessages, markMessageAsSent, getGfs } = require('./db');

const wsPort = 8080;    // WebSocket服务器的端口
const expressPort = 8081; // Express服务器的端口
const userConnections = new Map();  // 用户连接映射

let gfs;

// 下载文件的路由
app.get('/file/:id', async (req, res) => {
    try {

        const fileId = req.params.id;
        console.log(`Request for file ID: ${fileId}`);

        const objectId = new ObjectId(fileId);

        // console.log(gfs);

        // 打开 GridFS 下载流
        const downloadStream = gfs.openDownloadStream(objectId);

        downloadStream.on('data', (chunk) => {
            res.write(chunk);
        });

        downloadStream.on('end', () => {
            res.end();
        });

        downloadStream.on('error', (err) => {
            res.status(404).send('File not found');
        });
    } catch (error) {
        console.error('Server error:', error);
        res.status(500).send('Server error');
    }
});

// 创建 WebSocket 服务器，并在端口上监听
const webSocketServer = new WebSocket.Server({ port: wsPort }, () => {
    console.log(`WebSocket server listening on ws://${HOST}:${wsPort}`);
});

async function startServer() {

    try {
        // 连接到 MongoDB
        await connectToDb();

        gfs = getGfs();

        // 处理客户端连接
        webSocketServer.on('connection', (clientSocket, request) => {
            const clientIp = request.socket.remoteAddress;
            const clientPort = request.socket.remotePort;

            console.log('A new client '
                + clientIp + ':' + clientPort
                + ' has connected');

            // 处理收到的消息
            clientSocket.on('message', async (receivedMessage) => {
                if (Buffer.isBuffer(receivedMessage)) {
                    var units = ['B', 'KB', 'MB', 'GB', 'TB'];
                    var size = receivedMessage.length;

                    var unit = units[0];
                    var unitIndex = 0;

                    while (size > 1024 && unitIndex < units.length - 1) {
                        size = size / 1024;
                        unitIndex++;
                        unit = units[unitIndex];
                    }

                    console.log('Received binary message from client of length '
                        + size.toFixed(2) + ' ' + unit);

                    // 处理消息
                    await processMessage(receivedMessage, clientSocket);

                }
                else {
                    console.log('Received unknown message from client: ', receivedMessage);
                }
            });

            // 处理客户端断开连接
            clientSocket.on('close', () => {
                console.log('Client '
                    + clientIp + ':' + clientPort
                    + 'has disconnected');

                // 从用户连接映射中移除
                for (let [userId, socket] of userConnections) {
                    if (socket === clientSocket) {
                        userConnections.delete(userId);
                        break;
                    }
                }
            });

            // 处理 WebSocket 错误
            clientSocket.on('error', (error) => {
                console.log('WebSocket error: ', error);
            });
        });

    } catch (error) {
        console.log('Error starting server: ', error);
    }

}

async function processMessage(message, clientSocket) {

    try {
        const jsonData = JSON.parse(message);

        console.log("jsonData: " + JSON.stringify(jsonData, null, 2));

        // 连接成功后客户端会发送cliendtID以表示身份
        if (jsonData.type === 'clientId') {
            // 往连接映射中添加用户连接
            userConnections.set(jsonData.value, clientSocket);

            // 检查是否有未发送的消息
            const unsentMessages = await getUnsentMessages(jsonData.value);

            for (let message of unsentMessages) {
                let jsonString;
                if (message.type === 'text') {
                    jsonString = JSON.stringify(message);
                }
                else if (message.type === 'image' || message.type === 'file') {
                    const fileId = message.fileId;

                    const fileUrl = `http://${HOST}:8081/file/${fileId}`;

                    jsonString = JSON.stringify({
                        type: message.type,
                        senderId: message.senderId,
                        receiverId: message.receiverId,
                        time: message.time,
                        fileName: message.fileName,
                        fileSize: message.fileSize,
                        fileUrl: fileUrl
                    });
                }

                const binaryData = Buffer.from(jsonString);
                clientSocket.send(binaryData);

                // 标记为已发送
                markMessageAsSent(message._id);
            }

            return;
        }

        // 转发消息到目标用户
        const targetSocket = userConnections.get(jsonData.receiverId);

        if (targetSocket) {

            if (jsonData.type === 'text') {
                // console.log('Text message received:', jsonData);

                // 保存数据至数据库，标记为已发送
                await saveText(jsonData, true);

                targetSocket.send(message);

                console.log('Message forwarded to target user');
                
            }
            else if (jsonData.type === 'image' || jsonData.type === 'file') {
                // console.log('Image or filemessage received:', jsonData);

                // 保存数据至数据库，标记为已发送
                const fileId = await saveFile(jsonData, true);

                const fileUrl = `http://${HOST}:8081/file/${fileId}`;

                const jsonString = JSON.stringify({
                    type: jsonData.type,
                    senderId: jsonData.senderId,
                    receiverId: jsonData.receiverId,
                    time: jsonData.time,
                    fileName: jsonData.metadata.fileName,
                    fileSize: jsonData.metadata.fileSize,
                    fileUrl: fileUrl
                });

                const binaryData = Buffer.from(jsonString);

                targetSocket.send(binaryData);
            }

        } else {
            console.log('Target user is not connected');

            if (jsonData.type === 'text') {
                // 保存数据至数据库，标记为未发送
                saveText(jsonData, false);
            }
            else if (jsonData.type === 'image' || jsonData.type === 'file') {
                // 保存数据至数据库，标记为未发送
                saveFile(jsonData, false);
            }
        }

    } catch(error) {
        console.log('Error processing message: ', error);
    }
}

// 启动WebSocket服务器
startServer().catch(console.error);

// 启动Express服务器
app.listen(expressPort, () => {
    console.log(`File server is running on http://${HOST}:${expressPort}`);
});

// 监听进程退出事件，关闭 MongoDB 连接
process.on('SIGINT', async () => {
    await closeConnection();
    process.exit();
});


