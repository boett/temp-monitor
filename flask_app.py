
from flask import Flask, request, jsonify, redirect
import MySQLdb

app = Flask(__name__)
app.config.from_object('temp_config.Config')

def connect_db():
    db=MySQLdb.connect(host=app.config['MYSQL_HOST'],
                       user=app.config['MYSQL_USER'],
                       passwd=app.config['MYSQL_PASSWD'],
                       db=app.config['MYSQL_DB'])
    return db

@app.route('/')
def root():
    return redirect('/static/plot.html')

@app.route('/get_temps')
def get_temps():
    db=connect_db()
    cursor = db.cursor()
    cursor.execute("SELECT DATE_FORMAT( CONVERT_TZ(time, 'UTC', 'America/New_York'), '%Y-%m-%d %k:%i:%s') as t, id, temp from temps")

    data= [ {'x': [], 'y': []}, {'x': [], 'y': []}, {'x': [], 'y': []} ]
    for d in cursor:
        data[d[1]-1]['x'].append(d[0])
        data[d[1]-1]['y'].append(d[2])

#    fans = [ {'x': [], 'y': []}, {'x': [], 'y': []} ]
#    cursor.execute("SELECT DATE_FORMAT( CONVERT_TZ(time, 'UTC', 'America/New_York'), '%Y-%m-%d %k:%i:%s') as t, fan0, fan1 from fans")
#    for d in cursor:
#        fans[0]['x'].append(d[0])
#        fans[1]['x'].append(d[0])
#        fans[0]['y'].append(d[1])
#        fans[1]['y'].append(d[2])

    return jsonify(temps=data)
#    return app.response_class(data, mimetype='application/json')

@app.route('/update', methods=['GET', 'POST'])
def update():
    print(request.form)
    return 'updated'

@app.route('/post_temps', methods=['GET', 'POST'])
def post_temps():
    db=connect_db()
    #print(request.data)
    js = request.get_json(force=True)
    #print(js)
    #print request.headers

    for s in js['temperatures']:
        sn = s['sn']
        t = s['temp']
        data = None
        while not data:
            cursor = db.cursor()
            cursor.execute("SELECT id from sensors where sn = '%s'" % sn)
            data = cursor.fetchone()
            if data:
                id = data[0]
            else:
                cursor = db.cursor()
                cursor.execute("INSERT INTO sensors (sn) VALUES ('%s')" % sn)

        cursor = db.cursor()
        cursor.execute("INSERT INTO temps (id, temp) VALUES (%i, %f)" % (id, float(t)))
        db.commit()

    return jsonify({'fanstate': [1, 0]})

@app.route('/initdb')
def initdb():
    return

    db=connect_db()
    cursor = db.cursor()
    cursor.execute("DROP TABLE IF EXISTS temps;")
    cursor.execute("DROP TABLE IF EXISTS sensors;")
    cursor.execute("DROP TABLE IF EXISTS log;")

    cursor.execute("CREATE TABLE temps (time DATETIME DEFAULT NOW(), id INT, temp REAL);")
    cursor.execute("CREATE TABLE sensors (id INT NOT NULL AUTO_INCREMENT, sn CHAR(16), PRIMARY KEY (id));")
    cursor.execute("CREATE TABLE log (time DATETIME DEFAULT NOW(), msg VARCHAR(256));")

    db.commit()
    return 'init'

