// Copyright (C) 2019 Yasuhiro Matsumoto <mattn.jp@gmail.com>.
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// +build cgo

package sqlite3

import (
	"database/sql"
	"fmt"
	"os"
	"testing"
	"time"
)

// The number of rows of test data to create in the source database.
// Can be used to control how many pages are available to be backed up.
const testRowCount = 100

// The maximum number of seconds after which the page-by-page backup is considered to have taken too long.
const usePagePerStepsTimeoutSeconds = 30

// Test the backup functionality.
func testBackup(t *testing.T, testRowCount int, usePerPageSteps bool) {
	// This function will be called multiple times.
	// It uses sql.Register(), which requires the name parameter value to be unique.
	// There does not currently appear to be a way to unregister a registered driver, however.
	// So generate a database driver name that will likely be unique.
	var driverName = fmt.Sprintf("sqlite3_testBackup_%v_%v_%v", testRowCount, usePerPageSteps, time.Now().UnixNano())

	// The driver's connection will be needed in order to perform the backup.
	driverConns := []*SQLiteConn{}
	sql.Register(driverName, &SQLiteDriver{
		ConnectHook: func(conn *SQLiteConn) error {
			driverConns = append(driverConns, conn)
			return nil
		},
	})

	// Connect to the source database.
	srcTempFilename := TempFilename(t)
	defer os.Remove(srcTempFilename)
	srcDb, err := sql.Open(driverName, srcTempFilename)
	if err != nil {
		t.Fatal("Failed to open the source database:", err)
	}
	defer srcDb.Close()
	err = srcDb.Ping()
	if err != nil {
		t.Fatal("Failed to connect to the source database:", err)
	}

	// Connect to the destination database.
	destTempFilename := TempFilename(t)
	defer os.Remove(destTempFilename)
	destDb, err := sql.Open(driverName, destTempFilename)
	if err != nil {
		t.Fatal("Failed to open the destination database:", err)
	}
	defer destDb.Close()
	err = destDb.Ping()
	if err != nil {
		t.Fatal("Failed to connect to the destination database:", err)
	}

	// Check the driver connections.
	if len(driverConns) != 2 {
		t.Fatalf("Expected 2 driver connections, but found %v.", len(driverConns))
	}
	srcDbDriverConn := driverConns[0]
	if srcDbDriverConn == nil {
		t.Fatal("The source database driver connection is nil.")
	}
	destDbDriverConn := driverConns[1]
	if destDbDriverConn == nil {
		t.Fatal("The destination database driver connection is nil.")
	}

	// Generate some test data for the given ID.
	var generateTestData = func(id int) string {
		return fmt.Sprintf("test-%v", id)
	}

	// Populate the source database with a test table containing some test data.
	tx, err := srcDb.Begin()
	if err != nil {
		t.Fatal("Failed to begin a transaction when populating the source database:", err)
	}
	_, err = srcDb.Exec("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)")
	if err != nil {
		tx.Rollback()
		t.Fatal("Failed to create the source database \"test\" table:", err)
	}
	for id := 0; id < testRowCount; id++ {
		_, err = srcDb.Exec("INSERT INTO test (id, value) VALUES (?, ?)", id, generateTestData(id))
		if err != nil {
			tx.Rollback()
			t.Fatal("Failed to insert a row into the source database \"test\" table:", err)
		}
	}
	err = tx.Commit()
	if err != nil {
		t.Fatal("Failed to populate the source database:", err)
	}

	// Confirm that the destination database is initially empty.
	var destTableCount int
	err = destDb.QueryRow("SELECT COUNT(*) FROM sqlite_master WHERE type = 'table'").Scan(&destTableCount)
	if err != nil {
		t.Fatal("Failed to check the destination table count:", err)
	}
	if destTableCount != 0 {
		t.Fatalf("The destination database is not empty; %v table(s) found.", destTableCount)
	}

	// Prepare to perform the backup.
	backup, err := destDbDriverConn.Backup("main", srcDbDriverConn, "main")
	if err != nil {
		t.Fatal("Failed to initialize the backup:", err)
	}

	// Allow the initial page count and remaining values to be retrieved.
	// According to <https://www.sqlite.org/c3ref/backup_finish.html>, the page count and remaining values are "... only updated by sqlite3_backup_step()."
	isDone, err := backup.Step(0)
	if err != nil {
		t.Fatal("Unable to perform an initial 0-page backup step:", err)
	}
	if isDone {
		t.Fatal("Backup is unexpectedly done.")
	}

	// Check that the page count and remaining values are reasonable.
	initialPageCount := backup.PageCount()
	if initialPageCount <= 0 {
		t.Fatalf("Unexpected initial page count value: %v", initialPageCount)
	}
	initialRemaining := backup.Remaining()
	if initialRemaining <= 0 {
		t.F