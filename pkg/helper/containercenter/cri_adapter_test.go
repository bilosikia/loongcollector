// Copyright 2021 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//go:build linux
// +build linux

package containercenter

import (
	"testing"

	"github.com/docker/docker/api/types"

	"github.com/stretchr/testify/require"
)

func TestLookupContainerRootfsAbsDir(t *testing.T) {
	crirt := &CRIRuntimeWrapper{
		containerCenter: nil,
		client:          nil,
		runtimeInfo:     CriVersionInfo{},
		containers:      make(map[string]*innerContainerInfo),
		stopCh:          make(<-chan struct{}),
		rootfsCache:     make(map[string]string),
	}

	container := types.ContainerJSON{
		ContainerJSONBase: &types.ContainerJSONBase{
			ID: "1234567890abcde",
		},
	}
	dir := crirt.lookupContainerRootfsAbsDir(container)
	require.Equal(t, dir, "")
}
