// Copyright 2015 The Android Open Source Project
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

#ifndef KEYSTORE_KEYSTORE_CLIENT_H_
#define KEYSTORE_KEYSTORE_CLIENT_H_

#include <set>
#include <string>
#include <vector>

#include "hardware/keymaster_defs.h"
#include "keymaster/authorization_set.h"

namespace keystore {

// An abstract class providing a convenient interface to keystore services. This
// interface is designed to:
//   - hide details of the IPC mechanism (e.g. binder)
//   - use std data types
//   - encourage the use of keymaster::AuthorizationSet[Builder]
//   - be convenient for native services integrating with keystore
//   - be safely mocked for unit testing (e.g. pure virtual methods)
//
// Example usage:
//   KeystoreClient* keystore = new KeyStoreClientImpl();
//   keystore->AddRandomNumberGeneratorEntropy("unpredictable");
//
// Notes on error codes:
//   Keystore binder methods return a variety of values including ResponseCode
//   values defined in keystore.h, keymaster_error_t values defined in
//   keymaster_defs.h, or just 0 or -1 (both of which conflict with
//   keymaster_error_t). The methods in this class converge on a single success
//   indicator for convenience. KM_ERROR_OK was chosen over ::NO_ERROR for two
//   reasons:
//   1) KM_ERROR_OK is 0, which is a common convention for success, is the gmock
//      default, and allows error checks like 'if (error) {...'.
//   2) Although both pollute the global namespace, KM_ERROR_OK has a prefix per
//      C convention and hopefully clients can use this interface without
//      needing to include 'keystore.h' directly.
class KeystoreClient {
  public:
    KeystoreClient() = default;
    virtual ~KeystoreClient() = default;

    // Adds |entropy| to the random number generator. Returns KM_ERROR_OK on
    // success and a Keystore ResponseCode or keymaster_error_t on failure.
    virtual int32_t addRandomNumberGeneratorEntropy(const std::string& entropy) = 0;

    // Generates a key according to the given |key_parameters| and stores it with
    // the given |key_name|. The [hardware|software]_enforced_characteristics of
    // the key are provided on success. Returns KM_ERROR_OK on success. Returns
    // KM_ERROR_OK on success and a Keystore ResponseCode or keymaster_error_t on
    // failure.
    virtual int32_t generateKey(const std::string& key_name,
                                const keymaster::AuthorizationSet& key_parameters,
                                keymaster::AuthorizationSet* hardware_enforced_characteristics,
                                keymaster::AuthorizationSet* software_enforced_characteristics) = 0;

    // Provides the [hardware|software]_enforced_characteristics of a key
    // identified by |key_name|. Returns KM_ERROR_OK on success and a Keystore
    // ResponseCode or keymaster_error_t on failure.
    virtual int32_t
    getKeyCharacteristics(const std::string& key_name,
                          keymaster::AuthorizationSet* hardware_enforced_characteristics,
                          keymaster::AuthorizationSet* software_enforced_characteristics) = 0;

    // Imports |key_data| in the given |key_format|, applies the given
    // |key_parameters|, and stores it with the given |key_name|. The
    // [hardware|software]_enforced_characteristics of the key are provided on
    // success. Returns KM_ERROR_OK on success and a Keystore ResponseCode or
    // keymaster_error_t on failure.
    virtual int32_t importKey(const std::string& key_name,
                              const keymaster::AuthorizationSet& key_parameters,
                              keymaster_key_format_t key_format, const std::string& key_data,
                              keymaster::AuthorizationSet* hardware_enforced_characteristics,
                              keymaster::AuthorizationSet* software_enforced_characteristics) = 0;

    // Exports the public key identified by |key_name| to |export_data| using
    // |export_format|. Returns KM_ERROR_OK on success and a Keystore ResponseCode
    // or keymaster_error_t on failure.
    virtual int32_t exportKey(keymaster_key_format_t export_format, const std::string& key_name,
                              std::string* export_data) = 0;

    // Deletes the key identified by |key_name|. Returns KM_ERROR_OK on success
    // and a Keystore ResponseCode or keymaster_error_t on failure.
    virtual int32_t deleteKey(const std::string& key_name) = 0;

    // Deletes all keys owned by the caller. Returns KM_ERROR_OK on success and a
    // Keystore ResponseCode or keymaster_error_t on failure.
    virtual int32_t deleteAllKeys() = 0;

    // Begins a cryptographic operation (e.g. encrypt, sign) identified by
    // |purpose| using the key identified by |key_name| and the given
    // |input_parameters|. On success, any |output_parameters| and an operation
    // |handle| are populated. Returns KM_ERROR_OK on success and a Keystore
    // ResponseCode or keymaster_error_t on failure.
    virtual int32_t beginOperation(keymaster_purpose_t purpose, const std::string& key_name,
                                   const keymaster::AuthorizationSet& input_parameters,
                                   keymaster::AuthorizationSet* output_parameters,
                                   keymaster_operation_handle_t* handle) = 0;

    // Continues the operation associated with |handle| using the given
    // |input_parameters| and |input_data|. On success, the
    // |num_input_bytes_consumed|, any |output_parameters|, and any |output_data|
    // is populated. Returns KM_ERROR_OK on success and a Keystore ResponseCode or
    // keymaster_error_t on failure.
    virtual int32_t updateOperation(keymaster_operation_handle_t handle,
                                    const keymaster::AuthorizationSet& input_parameters,
                                    const std::string& input_data, size_t* num_input_bytes_consumed,
                                    keymaster::AuthorizationSet* output_parameters,
                                    std::string* output_data) = 0;

    // Finishes the operation associated with |handle| using the given
    // |input_parameters| and, if necessary, a |signature_to_verify|. On success,
    // any |output_parameters| and final |output_data| are populated. Returns
    // KM_ERROR_OK on success and a Keystore ResponseCode or keymaster_error_t on
    // failure.
    virtual int32_t finishOperation(keymaster_operation_handle_t handle,
                                    const keymaster::AuthorizationSet& input_parameters,
                                    const std::string& signature_to_verify,
                                    keymaster::AuthorizationSet* output_parameters,
                                    std::string* output_data) = 0;

    // Aborts the operation associated with |handle|. Returns KM_ERROR_OK on
    // success and a Keystore ResponseCode or keymaster_error_t on failure.
    virtual int32_t abortOperation(keymaster_operation_handle_t handle) = 0;

    // Returns true if a key identified by |key_name| exists in the caller's
    // key store. Returns false if an error occurs.
    virtual bool doesKeyExist(const std::string& key_name) = 0;

    // Provides a |key_name_list| containing all existing key names in the
    // caller's key store starting with |prefix|. Returns true on success.
    virtual bool listKeys(const std::string& prefix, std::vector<std::string>* key_name_list) = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(KeystoreClient);
};

}  // namespace keystore

#endif  // KEYSTORE_KEYSTORE_CLIENT_H_