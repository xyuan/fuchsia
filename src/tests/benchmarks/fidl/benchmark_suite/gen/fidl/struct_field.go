// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package fidl

import (
	"fmt"
	"gen/config"
	"gen/fidl/util"
	"gen/types"
)

func init() {
	util.Register(config.FidlFile{
		Filename: "struct_field.gen.test.fidl",
		Gen:      fidlGenStructField,
		Definitions: []config.Definition{
			{
				Config: config.Config{
					"size": 16,
				},
			},
			{
				Config: config.Config{
					"size": 256,
				},
			},
			{
				Config: config.Config{
					"size": 4096,
				},
				Denylist: []config.Binding{config.Rust},
			},
		},
	})
}

func fidlGenStructField(config config.Config) (string, error) {
	size := config.GetInt("size")
	return fmt.Sprintf(`
struct StructField%[1]d {
%[2]s
};`, size, util.StructFields(types.Uint8, "field", size)), nil
}
