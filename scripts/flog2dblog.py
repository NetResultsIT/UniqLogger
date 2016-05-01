import os
import sqlite3
import sys
import fileinput
import re
import datetime

TESTDB = "example.db"


def open_db(filename):
    global conn
    conn = sqlite3.connect('example.db')


def create_database():
    c = conn.cursor()

    c.execute("CREATE TABLE IF NOT EXISTS  ul_level (\
                    id          INTEGER PRIMARY KEY,\
                    level_name  VARCHAR(40),\
                    level_value INTEGER UNIQUE\
               )")

    c.execute("CREATE TABLE IF NOT EXISTS  ul_event (\
                    id          INTEGER PRIMARY KEY,\
                    tstamp      TIMESTAMP,\
                    module      VARCHAR(40) NOT NULL,\
                    level       INTEGER NOT NULL REFERENCES ul_level(level_value),\
                    message		VARCHAR(5000)\
              )")

    errdict = {"FATAL": 0, "CRITICAL": 10, "WARNING": 100, "INFO": 1000, "DEBUG": 10000, "FULL DEBUG": 100000}

    for key, value in errdict.iteritems():
        c.execute("INSERT INTO ul_level (level_name, level_value) VALUES (\"" + key + "\", " + str(value) + ")")

    return


def close_db():
    global conn
    conn.commit()
    conn.close()


def insert_data(m):
    global conn
    c = conn.cursor()

    c.execute("INSERT INTO  ul_event (tstamp, module, level,\
                    message) VALUES (\"" + m.group(1) + "\", \"" + m.group(2) + "\", (select level_value from ul_level where level_name = '" + m.group(3) + "'), \"" + m.group(4) + "\")")

    return


def migrate_file(lfile):
    global conn

    print("here " + os.getcwd())
    linecount = 0
    for line in fileinput.input(lfile):
        linecount += 1
        _re = re.compile("^\[(.*)\] \[(.*)\] \[(.*)\] (.*)")
        m = _re.match(line)
        if m:
            print ("MATCHED line with the following tokens=", m.group(1))
            insert_data(m)
        else:
            print line, # this goes to the current file
    return linecount


if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Bad Usage: flog2dblog <logfile> <dbfile>")
        sys.exit(1)

    logfile = sys.argv[1]
    dbfile = sys.argv[2]

    if not os.path.isfile(logfile):
        print("No such file: " + logfile)
        sys.exit(2)

    open_db(dbfile)
    if not os.path.isfile(dbfile):
        create_database()
    t0 = datetime.datetime.now()
    lc = migrate_file(logfile)
    t1 = datetime.datetime.now()
    elapsed = t1 - t0
    print("Time elapsed (usecs) to migrate %d entries: %s" % (lc, elapsed.microseconds) )
    close_db()
    sys.exit(0)
