"""Small Flask app standing in for the University Result Portal."""
import os
import socket
from flask import Flask, jsonify

app = Flask(__name__)
RESULTS = {"1001": {"name": "Sample Student", "gpa": 8.4}}


@app.route("/")
def index():
    return "<h1>SIMATS Result Portal</h1><p>Served by: %s</p>" % socket.gethostname()


@app.route("/health")
def health():
    return jsonify(status="ok")


@app.route("/result/<regno>")
def result(regno):
    return jsonify(RESULTS.get(regno, {"error": "not found"}))


@app.route("/dbcheck")
def dbcheck():
    host = os.environ.get("DB_HOST", "db")
    try:
        socket.create_connection((host, 5432), timeout=2).close()
        return jsonify(db="reachable", host=host)
    except OSError as exc:
        return jsonify(db="unreachable", error=str(exc)), 503
