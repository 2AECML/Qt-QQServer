const { MongoClient, GridFSBucket, ObjectId } = require('mongodb');
const { Readable } = require('stream');

// MongoDB 连接字符串和数据库名称
const mongoUri = 'mongodb://localhost:27017';
const dbName = 'chatDB';

let client; // MongoDB 客户端实例
let db; // MongoDB 数据库实例
let gfs; // GridFSBucket 实例，用于处理文件存储

// 连接到 MongoDB 数据库
async function connectToDb() {

    try {
        if (!client) {
            client = new MongoClient(mongoUri);
            await client.connect();
            db = client.db(dbName); // 获取指定的数据库实例
            gfs = new GridFSBucket(db, { bucketName: 'uploads' });  // 初始化 GridFSBucket 实例，用于存储大文件
            console.log('Connected to MongoDB and GridFS');
        }
        // console.log('gfs initialized:', gfs);
    } catch (error) {
        console.error('Failed to connect to MongoDB', error);
    }

}

// 保存文本到 MongoDB
async function saveText(jsonData, isSent) {
    const collection = db.collection('messages');   // 获取 'messages' 集合
    const messageDocument = {
        type: jsonData.type,
        senderId: jsonData.senderId,
        receiverId: jsonData.receiverId,
        time: jsonData.time,
        content: jsonData.content,
        font: jsonData.font,
        isSent: isSent
    };

    await collection.insertOne(messageDocument);    // 将消息插入到集合中
    console.log('Saved text message to database: ' + messageDocument);
}

// 全局缓存对象，用于保存正在处理的文件分块数据
const pendingFiles = {};

// 保存文件到 MongoDB 的 GridFS
async function saveFile(jsonData, isSent) {
    const { metadata, chunk, type, senderId, receiverId, time } = jsonData;
    const { fileName, fileSize, chunkNumber, totalChunks } = metadata;

    console.log('Received file chunk:', fileName, chunkNumber, totalChunks);

    return new Promise(async (resolve, reject) => {
        try {
            if (!gfs) {
                console.error('GridFS not initialized');
                return;
            }
            
            // 如果还没有缓存文件分块，则初始化缓存
            if (!pendingFiles[fileName]) {
                pendingFiles[fileName] = {
                    chunks: Array(totalChunks).fill(null),
                    totalChunks: totalChunks
                };
            }
    
            // 将接收到的分块数据保存到缓存中
            pendingFiles[fileName].chunks[chunkNumber] = Buffer.from(chunk, 'base64');
    
            // 检查是否已接收到所有分块
            if (pendingFiles[fileName].chunks.every(chunk => chunk !== null)) {
                // 创建 GridFS 上传流
                const uploadStream = gfs.openUploadStream(fileName);
    
                // 将所有分块合并为一个 Buffer 对象
                const fileBuffer = Buffer.concat(pendingFiles[fileName].chunks);
    
                // 将文件数据写入 GridFS
                const readable = new Readable();
                readable.push(fileBuffer);
                readable.push(null); // 结束流
    
                // 将文件保存到 GridFS
                const fileId = uploadStream.id;
                readable.pipe(uploadStream);
    
                uploadStream.on('finish', async () => {
                    console.log('File saved to GridFS:', fileName);
    
                    // 保存消息的基本信息到 'messages' 集合
                    const messageCollection = db.collection('messages');
                    const messageDocument = {
                        type: type,
                        senderId: senderId,
                        receiverId: receiverId,
                        time: time,
                        fileId: fileId, // 文件在 GridFS 中的引用
                        fileName: fileName,
                        fileSize: fileSize,
                        isSent: isSent
                    };
    
                    await messageCollection.insertOne(messageDocument);
                    console.log('Basic Message Informations saved to database:', messageDocument);
    
                    // 清理缓存
                    delete pendingFiles[fileName];
                    resolve(fileId);  // 返回文件ID
                });
    
                uploadStream.on('error', (error) => {
                    console.error('Save file to GridFS failed:', error);
                    reject(error);
                });
            }
    
        } catch (error) {
            console.error('Error processing file chunk:', error);
        }
    });
}

async function getUnsentMessages(clientId) {
    try {
        const collection = db.collection('messages');
        // 获取未发送的信息，按时间升序排序
        const cursor = collection.find({ receiverId: clientId, isSent: false }).sort({ timestamp: 1 });
        const messages = await cursor.toArray();
        return messages;
    } catch (error) {
        console.error('Error fetching unsent messages:', error);
    }
}

async function markMessageAsSent(messageId) {
    try {
        const collection = db.collection('messages');
        const updateResult = await collection.updateOne(
            { _id: messageId },
            { $set: { isSent: true } }
        );
        console.log('Marked message as sent:', updateResult);

    } catch (error) {
        console.error('Error marking message as sent:', error);
    }
}

// 关闭 MongoDB 连接
async function closeConnection() {
    if (client) {
        await client.close();
        console.log('MongoDB connection closed');
    }
}

module.exports = {
    connectToDb,
    saveText,
    saveFile,
    closeConnection,
    getUnsentMessages,
    markMessageAsSent,
    getGfs: () => gfs
};