// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_NATIVE_BACKEND_KWALLET_X_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_NATIVE_BACKEND_KWALLET_X_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_vector.h"
#include "base/nix/xdg_util.h"
#include "base/time/time.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/password_manager/password_store_x.h"
#include "chrome/browser/profiles/profile.h"

namespace autofill {
struct PasswordForm;
}

namespace base {
class Pickle;
class PickleIterator;
class WaitableEvent;
}

namespace dbus {
class Bus;
class ObjectProxy;
}

// NativeBackend implementation using KWallet.
class NativeBackendKWallet : public PasswordStoreX::NativeBackend {
 public:
  NativeBackendKWallet(LocalProfileId id,
                       base::nix::DesktopEnvironment desktop_env);

  ~NativeBackendKWallet() override;

  bool Init() override;

  // Implements NativeBackend interface.
  password_manager::PasswordStoreChangeList AddLogin(
      const autofill::PasswordForm& form) override;
  bool UpdateLogin(const autofill::PasswordForm& form,
                   password_manager::PasswordStoreChangeList* changes) override;
  bool RemoveLogin(const autofill::PasswordForm& form,
                   password_manager::PasswordStoreChangeList* changes) override;
  bool RemoveLoginsCreatedBetween(
      base::Time delete_begin,
      base::Time delete_end,
      password_manager::PasswordStoreChangeList* changes) override;
  bool RemoveLoginsSyncedBetween(
      base::Time delete_begin,
      base::Time delete_end,
      password_manager::PasswordStoreChangeList* changes) override;
  bool GetLogins(const autofill::PasswordForm& form,
                 ScopedVector<autofill::PasswordForm>* forms) override;
  bool GetAutofillableLogins(
      ScopedVector<autofill::PasswordForm>* forms) override;
  bool GetBlacklistLogins(ScopedVector<autofill::PasswordForm>* forms) override;

 protected:
  // Invalid handle returned by WalletHandle().
  static const int kInvalidKWalletHandle = -1;

  // Internally used by Init(), but also for testing to provide a mock bus.
  bool InitWithBus(scoped_refptr<dbus::Bus> optional_bus);

  // Deserializes a list of PasswordForms from the wallet.
  static ScopedVector<autofill::PasswordForm> DeserializeValue(
      const std::string& signon_realm,
      const base::Pickle& pickle);

 private:
  enum InitResult {
    INIT_SUCCESS,    // Init succeeded.
    TEMPORARY_FAIL,  // Init failed, but might succeed after StartKWalletd().
    PERMANENT_FAIL   // Init failed, and is not likely to work later either.
  };

  enum TimestampToCompare {
    CREATION_TIMESTAMP,
    SYNC_TIMESTAMP,
  };

  enum class BlacklistOptions { AUTOFILLABLE, BLACKLISTED };

  // Initialization.
  bool StartKWalletd();
  InitResult InitWallet();
  void InitOnDBThread(scoped_refptr<dbus::Bus> optional_bus,
                      base::WaitableEvent* event,
                      bool* success);

  // Overwrites |forms| with all credentials matching |signon_realm|. Returns
  // true on success.
  bool GetLoginsList(const std::string& signon_realm,
                     int wallet_handle,
                     ScopedVector<autofill::PasswordForm>* forms)
      WARN_UNUSED_RESULT;

  // Overwrites |forms| with all credentials matching |options|. Returns true on
  // success.
  bool GetLoginsList(BlacklistOptions options,
                     int wallet_handle,
                     ScopedVector<autofill::PasswordForm>* forms)
      WARN_UNUSED_RESULT;

  // Overwrites |forms| with all stored credentials. Returns true on success.
  bool GetAllLogins(int wallet_handle,
                    ScopedVector<autofill::PasswordForm>* forms)
      WARN_UNUSED_RESULT;

  // Writes a list of PasswordForms to the wallet with the given signon_realm.
  // Overwrites any existing list for this signon_realm. Removes the entry if
  // |forms| is empty. Returns true on success.
  bool SetLoginsList(const std::vector<autofill::PasswordForm*>& forms,
                     const std::string& signon_realm,
                     int wallet_handle);

  // Removes password created/synced in the time interval. Returns |true| if the
  // operation succeeded. |changes| will contain the changes applied.
  bool RemoveLoginsBetween(base::Time delete_begin,
                           base::Time delete_end,
                           TimestampToCompare date_to_compare,
                           password_manager::PasswordStoreChangeList* changes);

  // Opens the wallet and ensures that the "Chrome Form Data" folder exists.
  // Returns kInvalidWalletHandle on error.
  int WalletHandle();

  // Generates a profile-specific folder name based on profile_id_.
  std::string GetProfileSpecificFolderName() const;

  // The local profile id, used to generate the folder name.
  const LocalProfileId profile_id_;

  // The KWallet folder name, possibly based on the local profile id.
  std::string folder_name_;

  // DBus handle for communication with klauncher and kwalletd.
  scoped_refptr<dbus::Bus> session_bus_;
  // Object proxy for kwalletd. We do not own this.
  dbus::ObjectProxy* kwallet_proxy_;

  // The name of the wallet we've opened. Set during Init().
  std::string wallet_name_;
  // The application name (e.g. "Chromium"), shown in KWallet auth dialogs.
  const std::string app_name_;

  // KWallet DBus name
  std::string dbus_service_name_;
  // DBus path to KWallet interfaces
  std::string dbus_path_;
  // The name used for logging and by klauncher when starting KWallet
  std::string kwalletd_name_;

  DISALLOW_COPY_AND_ASSIGN(NativeBackendKWallet);
};

#endif  // CHROME_BROWSER_PASSWORD_MANAGER_NATIVE_BACKEND_KWALLET_X_H_
