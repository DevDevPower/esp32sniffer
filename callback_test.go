// Copyright (C) 2019 Yasuhiro Matsumoto <mattn.jp@gmail.com>.
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

// +build cgo

package sqlite3

import (
	"errors"
	"math"
	"reflect"
	"testing"
)

func TestCallbackArgCast(t *testing.T) {
	intConv := callbackSyntheticForTests(reflect.ValueOf(int64(math.MaxInt64)), nil)
	floatConv := callbackSyntheticForTests(reflect.ValueOf(float64(math.MaxFloat64)), nil)
	errConv := callbackSyntheticForTests(reflect.Value{}, errors.New("test"))

	tests := []struct {
		f callbackArgConverter
		o reflect.Value
	}{
		{intConv, reflect.ValueOf(int8(-1))},
		{intConv, reflect.ValueOf(int16(-1))},
		{intConv, reflect.ValueOf(int32(-1))},
		{intConv, reflect.ValueOf(uint8(math.MaxUint8))},
		{intConv, reflect.ValueOf(uint16(math.MaxU