// Copyright 2020 The Netstack Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package dns

import (
	"testing"
	"time"

	"github.com/google/go-cmp/cmp"
	"gvisor.dev/gvisor/pkg/tcpip"
)

const (
	incrementalTimeout    = 100 * time.Millisecond
	shortLifetime         = 1 * time.Nanosecond
	shortLifetimeTimeout  = 1 * time.Second
	middleLifetime        = 1 * time.Second
	middleLifetimeTimeout = 2 * time.Second
	longLifetime          = 1 * time.Hour
)

var (
	addr1 = tcpip.FullAddress{
		Addr: "\xfe\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",
		Port: defaultDNSPort,
	}
	// Address is the same as addr1, but differnt port.
	addr2 = tcpip.FullAddress{
		Addr: "\xfe\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",
		Port: defaultDNSPort + 1,
	}
	addr3 = tcpip.FullAddress{
		Addr: "\xfe\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02",
		Port: defaultDNSPort + 2,
	}
	// Should assume default port of 53.
	addr4 = tcpip.FullAddress{
		Addr: "\xfe\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03",
		NIC:  5,
	}
	addr5 = tcpip.FullAddress{
		Addr: "\x0a\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05",
		Port: defaultDNSPort,
	}
	addr6 = tcpip.FullAddress{
		Addr: "\x0a\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x06",
		Port: defaultDNSPort,
	}
	addr7 = tcpip.FullAddress{
		Addr: "\x0a\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07",
		Port: defaultDNSPort,
	}
	addr8 = tcpip.FullAddress{
		Addr: "\x0a\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08",
		Port: defaultDNSPort,
	}
	addr9 = tcpip.FullAddress{
		Addr: "\x0a\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x09",
		Port: defaultDNSPort,
	}
	addr10 = tcpip.FullAddress{
		Addr: "\x0a\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a",
		Port: defaultDNSPort,
	}
)

func containsFullAddress(list []tcpip.FullAddress, item tcpip.FullAddress) bool {
	for _, i := range list {
		if i == item {
			return true
		}
	}

	return false
}

func containsAddress(list []tcpip.Address, item tcpip.Address) bool {
	for _, i := range list {
		if i == item {
			return true
		}
	}

	return false
}

func TestGetServersCacheNoDuplicates(t *testing.T) {
	addr3 := addr3
	addr3.Port = defaultDNSPort
	addr4WithPort := addr4
	addr4WithPort.Port = defaultDNSPort

	d := makeServersConfig()

	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr2, addr2, addr3, addr4, addr8}, longLifetime)
	runtimeServers1 := []tcpip.Address{addr5.Addr, addr5.Addr, addr6.Addr, addr7.Addr}
	runtimeServers2 := []tcpip.Address{addr6.Addr, addr7.Addr, addr8.Addr, addr9.Addr}
	d.SetRuntimeServers([]*[]tcpip.Address{&runtimeServers1, &runtimeServers2})
	d.SetDefaultServers([]tcpip.Address{addr3.Addr, addr9.Addr, addr10.Addr, addr10.Addr})
	servers := d.GetServersCache()
	if !containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
	}
	if !containsFullAddress(servers, addr2) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
	}
	if !containsFullAddress(servers, addr3) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr3, servers)
	}
	if !containsFullAddress(servers, addr4WithPort) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr4WithPort, servers)
	}
	if !containsFullAddress(servers, addr5) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr5, servers)
	}
	if !containsFullAddress(servers, addr6) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr6, servers)
	}
	if !containsFullAddress(servers, addr7) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr7, servers)
	}
	if !containsFullAddress(servers, addr8) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr8, servers)
	}
	if !containsFullAddress(servers, addr9) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr9, servers)
	}
	if !containsFullAddress(servers, addr10) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr10, servers)
	}
	if l := len(servers); l != 10 {
		t.Errorf("got len(servers) = %d, want = 10; servers = %+v", l, servers)
	}
}

func TestGetServersCacheOrdering(t *testing.T) {
	addr4WithPort := addr4
	addr4WithPort.Port = defaultDNSPort

	d := makeServersConfig()

	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr2, addr3, addr4}, longLifetime)
	runtimeServers1 := []tcpip.Address{addr5.Addr, addr6.Addr}
	runtimeServers2 := []tcpip.Address{addr7.Addr, addr8.Addr}
	d.SetRuntimeServers([]*[]tcpip.Address{&runtimeServers1, &runtimeServers2})
	d.SetDefaultServers([]tcpip.Address{addr9.Addr, addr10.Addr})
	servers := d.GetServersCache()
	expiringServers := servers[:4]
	runtimeServers := servers[4:8]
	defaultServers := servers[8:]
	if !containsFullAddress(expiringServers, addr1) {
		t.Errorf("expected %+v to be in the expiring server cache, got = %+v", addr1, expiringServers)
	}
	if !containsFullAddress(expiringServers, addr2) {
		t.Errorf("expected %+v to be in the expiring server cache, got = %+v", addr2, expiringServers)
	}
	if !containsFullAddress(expiringServers, addr3) {
		t.Errorf("expected %+v to be in the expiring server cache, got = %+v", addr3, expiringServers)
	}
	if !containsFullAddress(expiringServers, addr4WithPort) {
		t.Errorf("expected %+v to be in the expiring server cache, got = %+v", addr4WithPort, expiringServers)
	}
	if !containsFullAddress(runtimeServers, addr5) {
		t.Errorf("expected %+v to be in the runtime server cache, got = %+v", addr5, runtimeServers)
	}
	if !containsFullAddress(runtimeServers, addr6) {
		t.Errorf("expected %+v to be in the runtime server cache, got = %+v", addr6, runtimeServers)
	}
	if !containsFullAddress(runtimeServers, addr7) {
		t.Errorf("expected %+v to be in the runtime server cache, got = %+v", addr7, runtimeServers)
	}
	if !containsFullAddress(runtimeServers, addr8) {
		t.Errorf("expected %+v to be in the runtime server cache, got = %+v", addr8, runtimeServers)
	}
	if !containsFullAddress(defaultServers, addr9) {
		t.Errorf("expected %+v to be in the default server cache, got = %+v", addr9, defaultServers)
	}
	if !containsFullAddress(defaultServers, addr10) {
		t.Errorf("expected %+v to be in the default server cache, got = %+v", addr10, defaultServers)
	}
	if l := len(servers); l != 10 {
		t.Errorf("got len(servers) = %d, want = 10; servers = %+v", l, servers)
	}
}

func TestRemoveAllServersWithNIC(t *testing.T) {
	addr3 := addr3
	addr3.NIC = addr4.NIC
	addr4WithPort := addr4
	addr4WithPort.Port = defaultDNSPort

	d := makeServersConfig()

	expectAllAddresses := func() {
		t.Helper()

		servers := d.GetServersCache()
		if !containsFullAddress(servers, addr1) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
		}
		if !containsFullAddress(servers, addr2) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
		}
		if !containsFullAddress(servers, addr3) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr3, servers)
		}
		if !containsFullAddress(servers, addr4WithPort) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr4WithPort, servers)
		}
		if !containsFullAddress(servers, addr5) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr5, servers)
		}
		if !containsFullAddress(servers, addr6) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr6, servers)
		}
		if !containsFullAddress(servers, addr7) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr7, servers)
		}
		if !containsFullAddress(servers, addr8) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr8, servers)
		}
		if !containsFullAddress(servers, addr9) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr9, servers)
		}
		if !containsFullAddress(servers, addr10) {
			t.Errorf("expected %+v to be in the server cache, got = %+v", addr10, servers)
		}
		if l := len(servers); l != 10 {
			t.Errorf("got len(servers) = %d, want = 10; servers = %+v", l, servers)
		}

		if t.Failed() {
			t.FailNow()
		}
	}

	d.SetDefaultServers([]tcpip.Address{addr5.Addr, addr6.Addr})
	runtimeServers1 := []tcpip.Address{addr7.Addr, addr8.Addr}
	runtimeServers2 := []tcpip.Address{addr9.Addr, addr10.Addr}
	d.SetRuntimeServers([]*[]tcpip.Address{&runtimeServers1, &runtimeServers2})
	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr2, addr3, addr4}, longLifetime)
	expectAllAddresses()

	// Should do nothing since a NIC is not specified.
	d.RemoveAllServersWithNIC(0)
	expectAllAddresses()

	// Should do nothing since none of the addresses are associated with a NIC
	// with ID 255.
	d.RemoveAllServersWithNIC(255)
	expectAllAddresses()

	// Should remove addr3 and addr4.
	d.RemoveAllServersWithNIC(addr4.NIC)
	servers := d.GetServersCache()
	if !containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
	}
	if !containsFullAddress(servers, addr2) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
	}
	if !containsFullAddress(servers, addr5) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr5, servers)
	}
	if !containsFullAddress(servers, addr6) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr6, servers)
	}
	if !containsFullAddress(servers, addr7) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr7, servers)
	}
	if !containsFullAddress(servers, addr8) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr8, servers)
	}
	if !containsFullAddress(servers, addr9) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr9, servers)
	}
	if !containsFullAddress(servers, addr10) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr10, servers)
	}
	if l := len(servers); l != 8 {
		t.Errorf("got len(servers) = %d, want = 8; servers = %+v", l, servers)
	}
}

func TestGetServersCache(t *testing.T) {
	d := makeServersConfig()

	addr3 := addr3
	addr3.NIC = addr4.NIC
	addr4WithPort := addr4
	addr4WithPort.Port = defaultDNSPort

	d.SetDefaultServers([]tcpip.Address{addr5.Addr, addr6.Addr})
	servers := d.GetServersCache()
	if !containsFullAddress(servers, addr5) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr5, servers)
	}
	if !containsFullAddress(servers, addr6) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr6, servers)
	}
	if l := len(servers); l != 2 {
		t.Errorf("got len(servers) = %d, want = 2; servers = %+v", l, servers)
	}

	if t.Failed() {
		t.FailNow()
	}

	runtimeServers1 := []tcpip.Address{addr7.Addr, addr8.Addr}
	runtimeServers2 := []tcpip.Address{addr9.Addr, addr10.Addr}
	d.SetRuntimeServers([]*[]tcpip.Address{&runtimeServers1, &runtimeServers2})
	servers = d.GetServersCache()
	if !containsFullAddress(servers, addr5) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr5, servers)
	}
	if !containsFullAddress(servers, addr6) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr6, servers)
	}
	if !containsFullAddress(servers, addr7) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr7, servers)
	}
	if !containsFullAddress(servers, addr8) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr8, servers)
	}
	if !containsFullAddress(servers, addr9) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr9, servers)
	}
	if !containsFullAddress(servers, addr10) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr10, servers)
	}
	if l := len(servers); l != 6 {
		t.Errorf("got len(servers) = %d, want = 6; servers = %+v", l, servers)
	}

	if t.Failed() {
		t.FailNow()
	}

	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr2, addr3, addr4}, longLifetime)
	servers = d.GetServersCache()
	if !containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
	}
	if !containsFullAddress(servers, addr2) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
	}
	if !containsFullAddress(servers, addr3) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr3, servers)
	}
	if !containsFullAddress(servers, addr4WithPort) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr4WithPort, servers)
	}
	if !containsFullAddress(servers, addr5) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr5, servers)
	}
	if !containsFullAddress(servers, addr6) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr6, servers)
	}
	if !containsFullAddress(servers, addr7) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr7, servers)
	}
	if !containsFullAddress(servers, addr8) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr8, servers)
	}
	if !containsFullAddress(servers, addr9) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr9, servers)
	}
	if !containsFullAddress(servers, addr10) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr10, servers)
	}
	if l := len(servers); l != 10 {
		t.Errorf("got len(servers) = %d, want = 10; servers = %+v", l, servers)
	}

	// Should get the same results since there were no updates.
	if diff := cmp.Diff(servers, d.GetServersCache()); diff != "" {
		t.Errorf("d.GetServersCache() mismatch (-want +got):\n%s", diff)
	}

	if t.Failed() {
		t.FailNow()
	}

	d.SetRuntimeServers(nil)
	servers = d.GetServersCache()
	if !containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
	}
	if !containsFullAddress(servers, addr2) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
	}
	if !containsFullAddress(servers, addr3) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr3, servers)
	}
	if !containsFullAddress(servers, addr4WithPort) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr4WithPort, servers)
	}
	if !containsFullAddress(servers, addr5) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr5, servers)
	}
	if !containsFullAddress(servers, addr6) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr6, servers)
	}
	if l := len(servers); l != 6 {
		t.Errorf("got len(servers) = %d, want = 6; servers = %+v", l, servers)
	}

	// Should remove addr3 and addr4.
	d.RemoveAllServersWithNIC(addr4.NIC)
	servers = d.GetServersCache()
	if !containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
	}
	if !containsFullAddress(servers, addr2) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
	}
	if !containsFullAddress(servers, addr5) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr5, servers)
	}
	if !containsFullAddress(servers, addr6) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr6, servers)
	}
	if l := len(servers); l != 4 {
		t.Errorf("got len(servers) = %d, want = 4; servers = %+v", l, servers)
	}

	if t.Failed() {
		t.FailNow()
	}

	d.SetDefaultServers(nil)
	servers = d.GetServersCache()
	if !containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
	}
	if !containsFullAddress(servers, addr2) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
	}
	if l := len(servers); l != 2 {
		t.Errorf("got len(servers) = %d, want = 2; servers = %+v", l, servers)
	}

	if t.Failed() {
		t.FailNow()
	}

	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr2}, 0)
	servers = d.GetServersCache()
	if l := len(servers); l != 0 {
		t.Errorf("got len(servers) = %d, want = 0; servers = %+v", l, servers)
	}
}

func TestExpiringServersDefaultDNSPort(t *testing.T) {
	d := makeServersConfig()

	addr4WithPort := addr4
	addr4WithPort.Port = defaultDNSPort

	d.UpdateExpiringServers([]tcpip.FullAddress{addr4}, longLifetime)
	servers := d.GetServersCache()
	if !containsFullAddress(servers, addr4WithPort) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr4WithPort, servers)
	}
	if l := len(servers); l != 1 {
		t.Errorf("got len(servers) = %d, want = 1; servers = %+v", l, servers)
	}
}

func TestExpiringServersUpdateWithDuplicates(t *testing.T) {
	d := makeServersConfig()

	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr1, addr1}, longLifetime)
	servers := d.GetServersCache()
	if !containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
	}
	if l := len(servers); l != 1 {
		t.Errorf("got len(servers) = %d, want = 1; servers = %+v", l, servers)
	}
}

func TestExpiringServersAddAndUpdate(t *testing.T) {
	d := makeServersConfig()

	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr2}, longLifetime)
	servers := d.GetServersCache()
	if !containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
	}
	if !containsFullAddress(servers, addr2) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
	}
	if l := len(servers); l != 2 {
		t.Errorf("got len(servers) = %d, want = 2; servers = %+v", l, servers)
	}

	if t.Failed() {
		t.FailNow()
	}

	// Refresh addr1 and addr2, add addr3.
	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr3, addr2}, longLifetime)
	servers = d.GetServersCache()
	if !containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr1, servers)
	}
	if !containsFullAddress(servers, addr2) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
	}
	if !containsFullAddress(servers, addr3) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr3, servers)
	}
	if l := len(servers); l != 3 {
		t.Errorf("got len(servers) = %d, want = 3; servers = %+v", l, servers)
	}

	if t.Failed() {
		t.FailNow()
	}

	// Lifetime of 0 should remove servers if they exist.
	d.UpdateExpiringServers([]tcpip.FullAddress{addr4, addr1}, 0)
	servers = d.GetServersCache()
	if containsFullAddress(servers, addr1) {
		t.Errorf("expected %+v to not be in the server cache, got = %+v", addr1, servers)
	}
	if containsFullAddress(servers, addr4) {
		t.Errorf("expected %+v to not be in the server cache, got = %+v", addr4, servers)
	}
	if !containsFullAddress(servers, addr2) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
	}
	if !containsFullAddress(servers, addr3) {
		t.Errorf("expected %+v to be in the server cache, got = %+v", addr3, servers)
	}
	if l := len(servers); l != 2 {
		t.Errorf("got len(servers) = %d, want = 2; servers = %+v", l, servers)
	}
}

func TestExpiringServersExpireImmediatelyTimer(t *testing.T) {
	t.Parallel()

	d := makeServersConfig()

	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr2}, shortLifetime)
	for elapsedTime := time.Duration(0); elapsedTime <= shortLifetimeTimeout; elapsedTime += incrementalTimeout {
		time.Sleep(incrementalTimeout)
		servers := d.GetServersCache()
		if l := len(servers); l != 0 {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Fatalf("got len(servers) = %d, want = 0; servers = %+v", l, servers)
		}

		break
	}
}

func TestExpiringServersExpireAfterUpdate(t *testing.T) {
	t.Parallel()

	d := makeServersConfig()

	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr2}, longLifetime)
	servers := d.GetServersCache()
	if l := len(servers); l != 2 {
		t.Fatalf("got len(servers) = %d, want = 2; servers = %+v", l, servers)
	}

	// addr2 and addr3 should expire, but addr1 should stay.
	d.UpdateExpiringServers([]tcpip.FullAddress{addr2, addr3}, shortLifetime)
	for elapsedTime := time.Duration(0); elapsedTime <= shortLifetimeTimeout; elapsedTime += incrementalTimeout {
		time.Sleep(incrementalTimeout)
		servers = d.GetServersCache()
		if !containsFullAddress(servers, addr1) {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("expected %+v to be in the server cache, got = %+v", addr2, servers)
		}
		if containsFullAddress(servers, addr2) {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("expected %+v to not be in the server cache, got = %+v", addr3, servers)
		}
		if containsFullAddress(servers, addr3) {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("expected %+v to not be in the server cache, got = %+v", addr3, servers)
		}
		if l := len(servers); l != 1 {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("got len(servers) = %d, want = 1; servers = %+v", l, servers)
		}

		break
	}
}

func TestExpiringServersInfiniteLifetime(t *testing.T) {
	t.Parallel()

	d := makeServersConfig()

	d.UpdateExpiringServers([]tcpip.FullAddress{addr1, addr2}, middleLifetime)
	servers := d.GetServersCache()
	if l := len(servers); l != 2 {
		t.Fatalf("got len(servers) = %d, want = 2; servers = %+v", l, servers)
	}

	// addr1 should expire, but addr2 and addr3 should be valid forever.
	d.UpdateExpiringServers([]tcpip.FullAddress{addr2, addr3}, -1)
	for elapsedTime := time.Duration(0); elapsedTime < middleLifetimeTimeout; elapsedTime += incrementalTimeout {
		time.Sleep(incrementalTimeout)
		servers = d.GetServersCache()
		if containsFullAddress(servers, addr1) {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("expected %+v to not be in the server cache, got = %+v", addr2, servers)
		}
		if !containsFullAddress(servers, addr2) {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("expected %+v to be in the server cache, got = %+v", addr3, servers)
		}
		if !containsFullAddress(servers, addr3) {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("expected %+v to be in the server cache, got = %+v", addr3, servers)
		}
		if l := len(servers); l != 2 {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("got len(servers) = %d, want = 2; servers = %+v", l, servers)
		}

		break
	}

	if t.Failed() {
		t.FailNow()
	}

	d.UpdateExpiringServers([]tcpip.FullAddress{addr2, addr3}, middleLifetime)
	for elapsedTime := time.Duration(0); elapsedTime <= middleLifetimeTimeout; elapsedTime += incrementalTimeout {
		time.Sleep(incrementalTimeout)
		servers = d.GetServersCache()
		if containsFullAddress(servers, addr1) {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("expected %+v to not be in the server cache, got = %+v", addr2, servers)
		}
		if containsFullAddress(servers, addr2) {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("expected %+v to not be in the server cache, got = %+v", addr3, servers)
		}
		if containsFullAddress(servers, addr3) {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("expected %+v to not be in the server cache, got = %+v", addr3, servers)
		}
		if l := len(servers); l != 0 {
			if elapsedTime < middleLifetimeTimeout {
				continue
			}

			t.Errorf("got len(servers) = %d, want = 0; servers = %+v", l, servers)
		}

		break
	}
}