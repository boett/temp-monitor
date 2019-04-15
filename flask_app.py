
from flask import Flask, request, jsonify, redirect, render_template
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
    data = []
    cursor.execute("SELECT id, name from sensors")
    for r in cursor:
        data.append( {'id': r[0], 'name': r[1], 'x': [], 'y': []})

    cursor.execute("SELECT DATE_FORMAT( CONVERT_TZ(time, 'UTC', 'America/New_York'), '%Y-%m-%d %k:%i:%s') as t, id, temp from temps")

    for d in cursor:
        idx = d[1] - 1
        data[idx]['x'].append(d[0])
        data[idx]['y'].append(d[2])

    return jsonify(temps=data)

@app.route('/post_temps', methods=['GET', 'POST'])
def post_temps():
    db=connect_db()
    js = request.get_json(force=True)

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

    return jsonify({'success': [1, 1]})

@app.route('/sensors')
def sensors():
    db=connect_db()
    cursor = db.cursor()
    cursor.execute("SELECT id, sn, name from sensors")
    sensors = [ {'id': r[0], 'sn': r[1], 'name': r[2]} for r in cursor ]
    return render_template('sensors.html', sensors=sensors)

@app.route('/sensoredit', methods=['GET', 'POST'])
def sensoredit():
    db=connect_db()
    res = request.get_json(force=True)

    for r in res['sensors']:
        cursor = db.cursor()
        cursor.execute("UPDATE sensors SET name=%s WHERE id=%s", (r[1],r[0]))

    db.commit()
    return jsonify({'success': 1})

@app.route('/initdb')
def initdb():
    return

    db=connect_db()
    cursor = db.cursor()
    cursor.execute("DROP TABLE IF EXISTS temps;")
    cursor.execute("DROP TABLE IF EXISTS sensors;")
    cursor.execute("DROP TABLE IF EXISTS log;")

    cursor.execute("CREATE TABLE temps (time DATETIME DEFAULT NOW(), id INT, temp REAL);")
    cursor.execute("CREATE TABLE sensors (id INT NOT NULL AUTO_INCREMENT, sn CHAR(16), name VARCHAR(256) PRIMARY KEY (id));")
    cursor.execute("CREATE TABLE log (time DATETIME DEFAULT NOW(), msg VARCHAR(256));")

    db.commit()
    return 'init'

