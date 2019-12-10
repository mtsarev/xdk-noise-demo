from flask import Flask, request, render_template
from flask_socketio import SocketIO, emit

app = Flask(__name__, template_folder='.')
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/data', methods=['POST'])
def data():
    socketio.emit('data', request.json)
    return '', 204

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=80)