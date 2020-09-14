package main

import (
	"fmt"

	"github.com/mattn/go-sqlite3"
)

type seriesModule struct{}

func (m *seriesModule) EponymousOnlyModule() {}

func (m *seriesModule) Create(c *sqlite3.SQLiteConn, args []string) (sqlite3.VTab, error) {
	err := c.DeclareVTab(fmt.Sprintf(`
		CREATE TABLE %s (
			value INT,
			start HIDDEN,
			stop HIDDEN,
			step HIDDEN
		)`, args[0]))
	if err != nil {
		return nil, err
	}
	return &seriesTable{0, 0, 1}, nil
}

func (m *seriesModule) Connect(c *sqlite3.SQLiteConn, args []string