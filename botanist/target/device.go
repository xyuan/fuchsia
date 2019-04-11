// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package target

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"net"
	"time"

	"fuchsia.googlesource.com/tools/botanist"
	"fuchsia.googlesource.com/tools/botanist/power"
	"fuchsia.googlesource.com/tools/build"
	"fuchsia.googlesource.com/tools/netboot"
	"fuchsia.googlesource.com/tools/netutil"

	"golang.org/x/crypto/ssh"
)

const (
	// The duration we allow for the netstack to come up when booting.
	netstackTimeout = 90 * time.Second
)

// DeviceConfig contains the static properties of a target device.
type DeviceConfig struct {
	// Network is the network properties of the target.
	Network NetworkProperties `json:"network"`

	// Power is the attached power management configuration.
	Power *power.Client `json:"power,omitempty"`

	// SSHKeys are the default system keys to be used with the device.
	SSHKeys []string `json:"keys,omitempty"`

	// Serial is the path to the device file for serial i/o.
	Serial string `json:"serial,omitempty`
}

// NetworkProperties are the static network properties of a target.
type NetworkProperties struct {
	// Nodename is the hostname of the device that we want to boot on.
	Nodename string `json:"nodename"`

	// IPv4Addr is the IPv4 address, if statically given. If not provided, it may be
	// resolved via the netstack's MDNS server.
	IPv4Addr string `json:"ipv4"`
}

// LoadDeviceConfigs unmarshalls a slice of device configs from a given file.
func LoadDeviceConfigs(path string) ([]DeviceConfig, error) {
	data, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("failed to read device properties file %q", path)
	}

	var configs []DeviceConfig
	if err := json.Unmarshal(data, &configs); err != nil {
		return nil, fmt.Errorf("failed to unmarshal configs: %v", err)
	}
	return configs, nil
}

// DeviceTarget represents a target device.
type DeviceTarget struct {
	config  DeviceConfig
	opts    Options
	signers []ssh.Signer
}

// NewDeviceTarget returns a new device target with a given configuration.
func NewDeviceTarget(config DeviceConfig, opts Options) (*DeviceTarget, error) {
	// If an SSH key is specified in the options, prepend it the configs list so that it
	// corresponds to the authorized key that would be paved.
	if opts.SSHKey != "" {
		config.SSHKeys = append([]string{opts.SSHKey}, config.SSHKeys...)
	}
	signers, err := parseOutSigners(config.SSHKeys)
	if err != nil {
		return nil, fmt.Errorf("could not parse out signers from private keys: %v", err)
	}
	return &DeviceTarget{
		config:  config,
		opts:    opts,
		signers: signers,
	}, nil
}

// Nodename returns the name of the node.
func (t *DeviceTarget) Nodename() string {
	return t.config.Network.Nodename
}

// IPv4Addr returns the IPv4 address of the node. If not provided in the config, then it
// will be resolved against the target-side MDNS server.
func (t *DeviceTarget) IPv4Addr() (net.IP, error) {
	if t.config.Network.IPv4Addr != "" {
		return net.ParseIP(t.config.Network.IPv4Addr), nil
	}
	return botanist.ResolveIPv4(context.Background(), t.Nodename(), netstackTimeout)
}

// Serial returns the serial device associated with the target for serial i/o.
func (t *DeviceTarget) Serial() string {
	return t.config.Serial
}

// SSHKey returns the private SSH key path associated with the authorized key to be paved.
func (t *DeviceTarget) SSHKey() string {
	return t.config.SSHKeys[0]
}

// Start starts the device target.
func (t *DeviceTarget) Start(ctx context.Context, images build.Images, args []string) error {
	if t.opts.Fastboot != "" {
		zirconR := images.Get("zircon-r")
		if zirconR == nil {
			return fmt.Errorf("zircon-r not provided")
		}
		// If it can't find any fastboot device, the fastboot tool will hang
		// waiting, so we add a timeout.  All fastboot operations take less
		// than a second on a developer workstation, so two minutes to flash
		// and continue is very generous.
		ctx, cancel := context.WithTimeout(ctx, 2*time.Minute)
		defer cancel()
		if err := botanist.FastbootToZedboot(ctx, t.opts.Fastboot, zirconR.Path); err != nil {
			return fmt.Errorf("failed to fastboot to zedboot: %v", err)
		}
	}

	// Set up log listener and dump kernel output to stdout.
	l, err := netboot.NewLogListener(t.Nodename())
	if err != nil {
		return fmt.Errorf("cannot listen: %v", err)
	}
	go func() {
		defer l.Close()
		for {
			data, err := l.Listen()
			if err != nil {
				continue
			}
			fmt.Print(data)
			select {
			case <-ctx.Done():
				return
			default:
			}
		}
	}()

	addr, err := netutil.GetNodeAddress(ctx, t.Nodename(), false)
	if err != nil {
		return err
	}

	// Boot Fuchsia.
	var bootMode int
	if t.opts.Netboot {
		bootMode = botanist.ModeNetboot
	} else {
		bootMode = botanist.ModePave
	}
	return botanist.Boot(ctx, addr, bootMode, images, args, t.signers)
}

// Restart restarts the target.
func (t *DeviceTarget) Restart(ctx context.Context) error {
	if t.config.Power != nil {
		if err := t.config.Power.RebootDevice(t.signers, t.Nodename()); err != nil {
			return fmt.Errorf("failed to reboot the device: %v", err)
		}
	}
	return nil
}

// Stop stops the device.
func (t *DeviceTarget) Stop(ctx context.Context) error {
	return ErrUnimplemented
}

// Wait waits for the device target to stop.
func (t *DeviceTarget) Wait(ctx context.Context) error {
	return ErrUnimplemented
}

func parseOutSigners(keyPaths []string) ([]ssh.Signer, error) {
	if len(keyPaths) == 0 {
		return nil, errors.New("must supply SSH keys in the config")
	}
	var keys [][]byte
	for _, keyPath := range keyPaths {
		p, err := ioutil.ReadFile(keyPath)
		if err != nil {
			return nil, fmt.Errorf("could not read SSH key file %q: %v", keyPath, err)
		}
		keys = append(keys, p)
	}

	var signers []ssh.Signer
	for _, p := range keys {
		signer, err := ssh.ParsePrivateKey(p)
		if err != nil {
			return nil, err
		}
		signers = append(signers, signer)
	}
	return signers, nil
}
