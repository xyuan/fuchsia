// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "inspector/loader.h"

#include <buffer/test_support/array_buffer.h>
#include <fs/journal/format.h>
#include <fs/transaction/block_transaction.h>
#include <minfs/format.h>
#include <zxtest/zxtest.h>

namespace minfs {
namespace {

class MockTransactionHandler : public fs::TransactionHandler {
 public:
  explicit MockTransactionHandler(storage::ArrayBuffer* mock_device) : mock_device_(mock_device) {}
  MockTransactionHandler(const MockTransactionHandler&) = delete;
  MockTransactionHandler(MockTransactionHandler&&) = default;
  MockTransactionHandler& operator=(const MockTransactionHandler&) = delete;
  MockTransactionHandler& operator=(MockTransactionHandler&&) = default;

  // TransactionHandler interface:
  uint32_t FsBlockSize() const final { return mock_device_->BlockSize(); }

  uint64_t BlockNumberToDevice(uint64_t block_num) const final { return block_num; }

  zx_status_t RunOperation(const storage::Operation& operation,
                           storage::BlockBuffer* buffer) final {
    ValidateOperation(operation, buffer);
    switch (operation.type) {
      case storage::OperationType::kRead:
        memcpy(buffer->Data(operation.vmo_offset), mock_device_->Data(operation.dev_offset),
               operation.length * mock_device_->BlockSize());
        break;
      case storage::OperationType::kWrite:
        memcpy(mock_device_->Data(operation.dev_offset), buffer->Data(operation.vmo_offset),
               operation.length * mock_device_->BlockSize());
        break;
      default:
        return ZX_ERR_NOT_SUPPORTED;
    }
    return ZX_OK;
  }

  groupid_t BlockGroupID() final { return 0; }

  uint32_t DeviceBlockSize() const final { return mock_device_->BlockSize(); }

  block_client::BlockDevice* GetDevice() final { return nullptr; }

  zx_status_t Transaction(block_fifo_request_t* requests, size_t count) final {
    return ZX_ERR_NOT_SUPPORTED;
  }

  void ValidateOperation(const storage::Operation& operation, storage::BlockBuffer* buffer) {
    ASSERT_NOT_NULL(mock_device_);
    ASSERT_GE(buffer->capacity(), operation.vmo_offset + operation.length,
              "Operation goes past input buffer length\n");
    ASSERT_GE(mock_device_->capacity(), operation.dev_offset + operation.length,
              "Operation goes past device buffer length\n");

    ASSERT_NE(operation.type, storage::OperationType::kTrim, "Trim operation is not supported\n");
  }

 private:
  storage::ArrayBuffer* mock_device_;
};

TEST(InspectorLoader, LoadSuperblock) {
  uint64_t start_block = 0;
  uint64_t block_length = 1;

  storage::ArrayBuffer device(block_length, kMinfsBlockSize);
  auto device_sb = reinterpret_cast<Superblock*>(device.Data(start_block));
  device_sb->magic0 = kMinfsMagic0;
  device_sb->magic1 = kMinfsMagic1;
  device_sb->dat_block = 1234;

  MockTransactionHandler handler(&device);
  Loader loader(&handler);

  storage::ArrayBuffer client_buffer(block_length, kMinfsBlockSize);
  ASSERT_OK(loader.LoadSuperblock(start_block, &client_buffer));
  EXPECT_BYTES_EQ(client_buffer.Data(0), device.Data(0), kMinfsBlockSize * block_length);
}

TEST(InspectorLoader, LoadInodeBitmap) {
  uint32_t start_block = 0;
  uint32_t block_length = 1;
  storage::ArrayBuffer device(block_length, kMinfsBlockSize);
  memset(device.Data(start_block), 'a', device.capacity() * device.BlockSize());

  Superblock superblock = {};
  superblock.ibm_block = start_block;
  superblock.abm_block = start_block + block_length;

  MockTransactionHandler handler(&device);
  Loader loader(&handler);

  storage::ArrayBuffer client_buffer(block_length, kMinfsBlockSize);
  ASSERT_OK(loader.LoadInodeBitmap(superblock, &client_buffer));
  EXPECT_BYTES_EQ(client_buffer.Data(0), device.Data(0), kMinfsBlockSize * block_length);
}

TEST(InspectorLoader, LoadInodeTable) {
  uint32_t start_block = 0;
  uint32_t block_length = 1;
  uint32_t inode_count = block_length * kMinfsInodesPerBlock;

  storage::ArrayBuffer device(block_length, kMinfsBlockSize);
  auto inodes = reinterpret_cast<Inode*>(device.Data(start_block));
  for (uint32_t i = 0; i < inode_count; ++i) {
    inodes[i].magic = kMinfsMagicFile;
    inodes[i].seq_num = i;
  }
  MockTransactionHandler handler(&device);
  Loader loader(&handler);

  Superblock superblock = {};
  superblock.inode_count = inode_count;
  superblock.ino_block = start_block;
  superblock.integrity_start_block = start_block + block_length;

  storage::ArrayBuffer client_buffer(block_length, kMinfsBlockSize);
  ASSERT_OK(loader.LoadInodeTable(superblock, &client_buffer));
  EXPECT_BYTES_EQ(client_buffer.Data(0), device.Data(0), kMinfsBlockSize * block_length);
}

TEST(InspectorLoader, LoadJournal) {
  uint32_t start_block = 0;
  // JournalInfo blocks and a single entry block.
  uint32_t block_length = fs::kJournalMetadataBlocks + 1;
  uint32_t device_length = block_length + kBackupSuperblockBlocks;

  storage::ArrayBuffer device(device_length, kMinfsBlockSize);
  auto journal =
      reinterpret_cast<fs::JournalInfo*>(device.Data(start_block + kBackupSuperblockBlocks));
  journal->magic = fs::kJournalMagic;

  MockTransactionHandler handler(&device);
  Loader loader(&handler);

  Superblock superblock = {};
  superblock.integrity_start_block = start_block;
  superblock.dat_block = start_block + device_length;

  storage::ArrayBuffer client_buffer(block_length, kMinfsBlockSize);
  ASSERT_OK(loader.LoadJournal(superblock, &client_buffer));
  EXPECT_BYTES_EQ(client_buffer.Data(0), device.Data(kBackupSuperblockBlocks),
                  kMinfsBlockSize * block_length);
}

}  // namespace
}  // namespace minfs
