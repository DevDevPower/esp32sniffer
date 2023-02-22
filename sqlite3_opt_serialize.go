// +build !libsqlite3 sqlite_serialize

package sqlite3

/*
#ifndef USE_LIBSQLITE3
#include <sqlite3-binding.h>
#else
#include <sqlite3.h>
#endif
#include <stdlib.h>
#include <stdint.h>
*/
import "C"

import (
	"fmt"
	"math"
	"reflect"
	"unsafe"
)

// Serialize returns a byte slice that is a serialization of the database.
//
// See https://www.sqlite.org/c3ref/serialize.html
func (c *SQLiteConn) Serialize(schema string) ([]byte, error) {
	if schema == "" {
		schema = "main"
	}
	var zSchema *C.char
	zSchema = C.CString(schema)
	defer C.free(unsafe.Pointer(zSchema))

	var sz C.sqlite3_int64
	ptr := C.sqlite3_serialize(c.db, zSchema, &sz, 0)
	if ptr == nil {
	