// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/upload_data_stream_builder.h"

#include <stdint.h>

#include <limits>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/browser/fileapi/upload_file_system_file_element_reader.h"
#include "content/common/resource_request_body.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_file_element_reader.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_reader.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/upload_blob_element_reader.h"

namespace disk_cache {
class Entry;
}

namespace content {
namespace {

// A subclass of net::UploadBytesElementReader which owns ResourceRequestBody.
class BytesElementReader : public net::UploadBytesElementReader {
 public:
  BytesElementReader(ResourceRequestBody* resource_request_body,
                     const ResourceRequestBody::Element& element)
      : net::UploadBytesElementReader(element.bytes(), element.length()),
        resource_request_body_(resource_request_body) {
    DCHECK_EQ(ResourceRequestBody::Element::TYPE_BYTES, element.type());
  }

  ~BytesElementReader() override {}

 private:
  scoped_refptr<ResourceRequestBody> resource_request_body_;

  DISALLOW_COPY_AND_ASSIGN(BytesElementReader);
};

// A subclass of net::UploadFileElementReader which owns ResourceRequestBody.
// This class is necessary to ensure the BlobData and any attached shareable
// files survive until upload completion.
class FileElementReader : public net::UploadFileElementReader {
 public:
  FileElementReader(ResourceRequestBody* resource_request_body,
                    base::TaskRunner* task_runner,
                    const ResourceRequestBody::Element& element)
      : net::UploadFileElementReader(task_runner,
                                     element.path(),
                                     element.offset(),
                                     element.length(),
                                     element.expected_modification_time()),
        resource_request_body_(resource_request_body) {
    DCHECK_EQ(ResourceRequestBody::Element::TYPE_FILE, element.type());
  }

  ~FileElementReader() override {}

 private:
  scoped_refptr<ResourceRequestBody> resource_request_body_;

  DISALLOW_COPY_AND_ASSIGN(FileElementReader);
};

}  // namespace

scoped_ptr<net::UploadDataStream> UploadDataStreamBuilder::Build(
    ResourceRequestBody* body,
    storage::BlobStorageContext* blob_context,
    storage::FileSystemContext* file_system_context,
    base::SingleThreadTaskRunner* file_task_runner) {
  std::vector<scoped_ptr<net::UploadElementReader>> element_readers;
  for (const auto& element : *body->elements()) {
    switch (element.type()) {
      case ResourceRequestBody::Element::TYPE_BYTES:
        element_readers.push_back(
            make_scoped_ptr(new BytesElementReader(body, element)));
        break;
      case ResourceRequestBody::Element::TYPE_FILE:
        element_readers.push_back(make_scoped_ptr(
            new FileElementReader(body, file_task_runner, element)));
        break;
      case ResourceRequestBody::Element::TYPE_FILE_FILESYSTEM:
        // If |body| contains any filesystem URLs, the caller should have
        // supplied a FileSystemContext.
        DCHECK(file_system_context);
        element_readers.push_back(
            make_scoped_ptr(new content::UploadFileSystemFileElementReader(
                file_system_context, element.filesystem_url(), element.offset(),
                element.length(), element.expected_modification_time())));
        break;
      case ResourceRequestBody::Element::TYPE_BLOB: {
        DCHECK_EQ(std::numeric_limits<uint64_t>::max(), element.length());
        DCHECK_EQ(0ul, element.offset());
        scoped_ptr<storage::BlobDataHandle> handle =
            blob_context->GetBlobDataFromUUID(element.blob_uuid());
        element_readers.push_back(
            make_scoped_ptr(new storage::UploadBlobElementReader(
                std::move(handle), file_system_context, file_task_runner)));
        break;
      }
      case ResourceRequestBody::Element::TYPE_DISK_CACHE_ENTRY:
      case ResourceRequestBody::Element::TYPE_BYTES_DESCRIPTION:
      case ResourceRequestBody::Element::TYPE_UNKNOWN:
        NOTREACHED();
        break;
    }
  }

  return make_scoped_ptr(new net::ElementsUploadDataStream(
      std::move(element_readers), body->identifier()));
}

}  // namespace content
