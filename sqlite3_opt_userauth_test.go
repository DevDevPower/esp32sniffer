// Copyright (C) 2018 G.J.R. Timmer <gjr.timmer@gmail.com>.
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// +build sqlite_userauth

package sqlite3

import (
	"database/sql"
	"fmt"
	"os"
	"testing"
)

var (
	conn             *SQLiteConn
	create           func(t *testing.T, username, password string) (file string, err error)
	createWithCrypt  func(t *testing.T, username, password, crypt, salt string) (file string, err error)
	connect          func(t *testing.T, f string, username, password string) (file string, db *sql.DB, c *SQLiteConn, err error)
	connectWithCrypt func(t *testing.T, f string, username, password string, crypt string, salt string) (file string, db *sql.DB, c *SQLiteConn, err error)
	authEnabled      func(db *sql.DB) (exists bool, err error)
	addUser          func(db *sql.DB, username, password string, admin int) (rv int, err error)
	userExists       func(db *sql.DB, username string) (rv int, err error)
	isAdmin          func(db *sql.DB, username string) (rv bool, err error)
	modifyUser       func(db *sql.DB, username, password string, admin int) (rv int, err error)
	deleteUser       func(db *sql.DB, username string) (rv int, err error)
)

func init() {
	// Create database connection
	sql.Register("sqlite3_with_conn",
		&SQLiteDriver{
			ConnectHook: func(c *SQLiteConn) error {
				conn = c
				return nil
			},
		})

	create = func(t *testing.T, username, password string) (file string, err error) {
		var db *sql.DB
		file, db, _, err = connect(t, "", username, password)
		db.Close()
		return
	}

	createWithCrypt = func(t *testing.T, username, password, crypt, salt string) (file string, err error) {
		var db *sql.DB
		file, db, _, err = connectWithCrypt(t, "", "admin", "admin", crypt, salt)
		db.Close()
		return
	}

	connect = func(t *testing.T, f string, username, password string) (file string, db *sql.DB, c *SQLiteConn, err error) {
		conn = nil // Clear connection
		file = f   // Copy provided file (f) => file
		if file == "" {
			// Create dummy file
			file = TempFilename(t)
		}

		params := "?_auth"
		if len(username) > 0 {
			params = fmt.Sprintf("%s&_auth_user=%s", params, username)
		}
		if len(password) > 0 {
			params = fmt.Sprintf("%s&_auth_pass=%s", params, password)
		}
		db, err = sql.Open("sqlite3_with_conn", "file:"+file+params)
		if err != nil {
			defer os.Remove(file)
			return file, nil, nil, err
		}

		// Dummy query to force connection and database creation
		// Will return ErrUnauthorized (SQLITE_AUTH) if user authentication fails
		if _, err = db.Exec("SELECT 1;"); err != nil {
			defer os.Remove(file)
			defer db.Close()
			return file, nil, nil, err
		}
		c = conn

		return
	}

	connectWithCrypt = func(t *testing.T, f string, username, password string, crypt string, salt string) (file string, db *sql.DB, c *SQLiteConn, err error) {
		conn = nil // Clear connection
		file = f   // Copy provided file (f) => file
		if file == "" {
			// Create dummy file
			file = TempFilename(t)
		}

		db, err = sql.Open("sqlite3_with_conn", "file:"+file+fmt.Sprintf("?_auth&_auth_user=%s&_auth_pass=%s&_auth_crypt=%s&_auth_salt=%s", username, password, crypt, salt))
		if err != nil {
			defer os.Remove(file)
			return file, nil, nil, err
		}

		// Dummy query to force connection and database creation
		// Will return ErrUnauthorized (SQLITE_AUTH) if user authentication fails
		if _, err = db.Exec("SELECT 1;"); err != nil {
			defer os.Remove(file)
			defer db.Close()
			return file, nil, nil, err
		}
		c = conn

		return
	}

	authEnabled = func(db *sql.DB) (exists bool, err error) {
		err = db.QueryRow("select count(type) from sqlite_master WHERE type='table' and name='sqlite_user';").Scan(&exists)
		return
	}

	addUser = func(db *sql.DB, username, password string, admin int) (rv int, err error) {
		err = db.QueryRow("select auth_user_add(?, ?, ?);", username, password, admin).Scan(&rv)
		return
	}

	userExists = func(db *sql.DB, username string) (rv int, err error) {
		err = db.QueryRow("select count(uname) from sqlite_user where uname=?", username).Scan(&rv)
		return
	}

	isAdmin = func(db *sql.DB, username string) (rv bool, err error) {
		err = db.QueryRow("select isAdmin from sqlite_user where uname=?", username).Scan(&rv)
		return
	}

	modifyUser = func(db *sql.DB, username, password string, admin int) (rv int, err error) {
		err = db.QueryRow("select auth_user_change(?, ?, ?);", username, password, admin).Scan(&rv)
		return
	}

	deleteUser = func(db *sql.DB, username string) (rv int, err error) {
		err = db.QueryRow("select auth_user_delete(?);", username).Scan(&rv)
		return
	}
}

func TestUserAuthCreateDatabase(t *testing.T) {
	f, db, c, err := connect(t, "", "admin", "admin")
	if err != nil && c == nil && db == nil {
		t.Fatal(err)
	}
	defer db.Close()
	defer os.Remove(f)

	enabled, err := authEnabled(db)
	if err != nil || !enabled {
		t.Fatalf("UserAuth not enabled: %s", err)
	}

	e, err := userExists(db, "admin")
	if err != nil {
		t.Fatal(err)
	}
	if e != 1 {
		t.Fatal("UserAuth: admin does not exists")
	}
	a, err := isAdmin(db, "admin")
	if err != nil {
		t.Fatal(err)
	}
	if !a {
		t.Fatal("UserAuth: User is not administrator")
	}
}

func TestUserAuthCreateDatabaseWithoutArgs(t *testing.T) {
	_, db, c, err := connect(t, "", "", "")
	if err == nil && c == nil && db == nil {
		t.Fatal("Should have failed due to missing _auth_* parameters")
	}

	_, db, c, err = connect(t, "", "", "admin")
	if err == nil && c == nil && db == nil {
		t.Fatal("Should have failed due to missing _auth_user parameter")
	}

	_, db, c, err = connect(t, "", "admin", "")
	if err == nil && c == nil && db == nil {
		t.Fatal("Should have failed due to missing _auth_pass parameter")
	}
}

func TestUserAuthLogin(t *testing.T) {
	f1, err := create(t, "admin", "admin")
	if err != nil {
		t.Fatal(err)
	}
	defer os.Remove(f1)

	f2, db2