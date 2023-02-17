// Copyright (C) 2019 Yasuhiro Matsumoto <mattn.jp@gmail.com>.
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// +build go1.13,cgo

package sqlite3

import (
	"context"
	"database/sql"
	"database/sql/driver"
	"errors"
	"os"
	"testing"
)

func TestBeginTxCancel(t *testing.T) {
	srcTempFilename := TempFilename(t)
	defer os.Remove(srcTempFilename)

	db, err := sql.Open("sqlite3", srcTempFilename)
	if err != nil {
		t.Fatal(err)
	}

	db.SetMaxOpenConns(10)
	db.SetMaxIdleConns(5)

	defer db.Close()
	initDatabase(t, db, 100)

	// create several go-routines to expose racy issue
	for i := 0; i <