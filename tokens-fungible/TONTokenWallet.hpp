/** \file
 *  \brief TONTokenWallet contract interfaces and data-structs.
 *
 *  Compiles into two contract versions: TONTokenWallet.tvc (external wallet) and FlexWallet.tvc (internal wallet).
 *  With different macroses.
 *  TIP3_ENABLE_EXTERNAL - enable external method calls (by key signature).
 *  TIP3_ENABLE_ALLOWANCE - enable allowance functionality.
 *  TIP3_ENABLE_LEND_OWNERSHIP - enable lend ownership functionality.
 *  TIP3_ENABLE_BURN - enable burn functionality, to transfer tokens back through Wrapper.
 *  TIP3_ENABLE_DESTROY - enable destroy method to self-destruct empty wallet.
 *  TIP3_DEPLOY_TRANSFER - enable transfer with deploy methods (transfer to recipient).
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include <tvm/schema/message.hpp>
#include <tvm/sequence.hpp>
#include <tvm/small_dict_map.hpp>

#include <tvm/replay_attack_protection/timestamp.hpp>
#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm {

// #define TIP3_ENABLE_EXTERNAL
// #define TIP3_ENABLE_ALLOWANCE
// #define TIP3_ENABLE_LEND_OWNERSHIP
// #define TIP3_ENABLE_BURN
// #define TIP3_ENABLE_DESTROY
// #define TIP3_ONLY_FLEX_EXTERNAL

#if defined(TIP3_ENABLE_EXTERNAL) && !defined(TIP3_ONLY_FLEX_EXTERNAL)
#define TIP3_EXTERNAL [[external]]
#else
#define TIP3_EXTERNAL
#endif

#if defined(TIP3_ENABLE_EXTERNAL) || defined(TIP3_ONLY_FLEX_EXTERNAL)
#define FLEX_EXTERNAL [[external]]
#else
#define FLEX_EXTERNAL
#endif

static constexpr unsigned TOKEN_WALLET_TIMESTAMP_DELAY = 1800;
using wallet_replay_protection_t = replay_attack_protection::timestamp<TOKEN_WALLET_TIMESTAMP_DELAY>;

/// Tip3 token configuration
struct Tip3Config {
  string  name;         ///< Token name.
  string  symbol;       ///< Token short symbol.
  uint8   decimals;     ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  uint256 root_pubkey;  ///< Public key of RootTokenContract for the tip3 token.
  address root_address; ///< Address of RootTokenContract for the tip3 token.
};

/// Allowance info.
struct allowance_info {
  address spender;         ///< Address of spender contract.
  uint128 remainingTokens; ///< Remaining amount of tokens for the spender.
};

/// Lend ownership pubkey record.
struct lend_pubkey {
  uint256 lend_pubkey;      ///< Lend pubkey.
  uint32  lend_finish_time; ///< Lend ownership finish time.
};

/// Complex key for lend owners mapping
struct lend_owner_key {
  addr_std_fixed dest;    ///< Destination contract address.
};

/// Lend ownership record (for usage in `address->lend_owner_record` map).
struct lend_owner {
  uint128 lend_balance;     ///< Lend ownership balance.
  uint32  lend_finish_time; ///< Lend ownership finish time.
  uint256 user_pubkey;      ///< Lend pubkey of the user ordered to lend ownership to the address
};
/// Lend owners (contracts) map
using lend_owners_map = small_dict_map<lend_owner_key, lend_owner>;

/// Lend ownership array record (for usage in getter).
struct lend_owner_array_record {
  lend_owner_key lend_key;         ///< Lend ownership key (destination address + user id).
  uint128        lend_balance;     ///< Lend ownership balance.
  uint32         lend_finish_time; ///< Lend ownership finish time.
};
/// Lend ownership array.
using lend_owners_array = dict_array<lend_owner_array_record>;

/// TONTokenWallet details info (for getter).
struct details_info {
  string            name;              ///< Token name.
  string            symbol;            ///< Token short symbol.
  uint8             decimals;          ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  uint128           balance;           ///< Token balance of the wallet.
  uint256           root_pubkey;       ///< Public key of the related RootTokenContract.
  address           root_address;      ///< Address of the related RootTokenContract.
  uint256           wallet_pubkey;     ///< Public key of wallet owner (User id for FlexWallet).
  address           owner_address;     ///< Owner contract address for internal ownership, will be 0:0..0 otherwise.
  opt<lend_pubkey>  lend_pubkey;       ///< Lend ownership pubkey.
  lend_owners_array lend_owners;       ///< All lend ownership records of the contract.
  uint128           lend_balance;      ///< Summarized lend balance to all targets.
                                       ///< Actual active balance will be `balance - lend_balance`.
  uint256           code_hash;         ///< Tip3 wallet code hash to verify other wallets.
  uint16            code_depth;        ///< Tip3 wallet code depth to verify other wallets.
  allowance_info    allowance;         ///< Allowance info.
  int8              workchain_id;      ///< Workchain id.
};

/** \interface ITONTokenWalletNotify
 *  \brief TON Token wallet notification callback interface.
 *
 *  Must be implemented in contracts receiving lend ownership or transfer notifications.
 */
__interface ITONTokenWalletNotify {

  /// Notification that target contract has received temporary (lend) ownership
  ///  of specified tokens amount in the wallet.
  [[internal, noaccept, answer_id]]
  bool onTip3LendOwnership(
    uint128     balance,          ///< Amount of tokens in temporary ownership of target contract.
                                  ///< \warning (for dev) balance must be first parameter to correctly process `on_bounced`.
    uint32      lend_finish_time, ///< Lend ownership finish time.
    uint256     pubkey,           ///< Pubkey of the wallet owner.
    address_opt owner,            ///< Owner contract address of the wallet if internal ownership is enabled.
    cell        payload,          ///< Payload (arbitrary cell).
    address     answer_addr       ///< Answer address (to receive answer and the remaining processing evers).
  ) = 201;

  /// Notification from tip3 wallet to its owner contract about received tokens transfer.
  [[internal, noaccept, answer_id]]
  bool onTip3Transfer(
    uint128     balance,       ///< New balance of the wallet.
    uint128     new_tokens,    ///< Amount of tokens received in transfer.
    uint256     sender_pubkey, ///< Sender wallet pubkey.
    address_opt sender_owner,  ///< Sender wallet internal owner.
    cell        payload,       ///< Payload (arbitrary cell).
    address     answer_addr    ///< Answer address (to receive answer and the remaining processing evers).
  ) = 202;
};
using ITONTokenWalletNotifyPtr = handle<ITONTokenWalletNotify>;

/** \interface ITONTokenWallet
 *  \brief TON Token wallet contract interface.
 */
__interface ITONTokenWallet {

  /// Transfer tokens to another tip3 wallet contract.
  TIP3_EXTERNAL
  [[internal, noaccept]]
  void transfer(
    address answer_addr,     ///< Answer address.
    address to,              ///< Destination tip3 wallet address.
    uint128 tokens,          ///< Amount of tokens to transfer.
    uint128 evers,           ///< Native funds to process.
                             ///< For internal requests, this value is ignored
                             ///<  and processing costs will be taken from attached value.
    uint128 return_ownership ///< Return ownership - to decrease lend ownership provided for the caller contract (additionally).
  ) = 10;

  /// Transfer with notify.
  /// Notify versions have answer_id to provide tail call answer
  ///  and send notification to the wallet owner contract.
  TIP3_EXTERNAL
  [[internal, noaccept, answer_id]]
  void transferWithNotify(
    address answer_addr,      ///< Answer address.
    address to,               ///< Destination tip3 wallet address.
    uint128 tokens,           ///< Amount of tokens to transfer.
    uint128 evers,            ///< Native funds to process.
                              ///< For internal requests, this value is ignored
                              ///<  and processing costs will be taken from attached value.
    uint128 return_ownership, ///< Return ownership - to decrease lend ownership provided for the caller contract (additionally).
    cell    payload           ///< Payload (arbitrary cell) - will be transmitted into owner's notification.
  ) = 11;

#ifdef TIP3_DEPLOY_TRANSFER
  /// Transfer to recipient.
  /// "ToRecipient" versions calculate destination wallet address.
  /// using recipient public key and recipient internal owner.
  TIP3_EXTERNAL
  [[internal, noaccept]]
  void transferToRecipient(
    address     answer_addr,      ///< Answer address.
    uint256     recipient_pubkey, ///< Recipient public key.
    address_opt recipient_owner,  ///< Recipient internal owner (contract address).
    uint128     tokens,           ///< Amount of tokens to transfer.
    uint128     evers,            ///< Native funds to process.
                                  ///< For internal requests, this value is ignored
                                  ///<  and processing costs will be taken from attached value.
    bool        deploy,           ///< Contract will send internalTransfer message with StateInit
                                  ///<  to also deploy new tip3 wallet (if it doesn't already exist) with
                                  ///<  the provided recipient_public_key and recipient_internal_owner.
    uint128     return_ownership  ///< Return ownership - to decrease lend ownership for the caller contract (additionally).
  ) = 12;

  /// Transfer to recipient with notify.
  /// "ToRecipient" versions calculate destination wallet address
  ///  using recipient public key and recipient internal owner.
  /// Notify versions have answer_id to provide tail call answer
  ///  and send notification to the wallet owner contract.
  TIP3_EXTERNAL
  [[internal, noaccept, answer_id]]
  void transferToRecipientWithNotify(
    address     answer_addr,      ///< Answer address.
    uint256     recipient_pubkey, ///< Recipient public key.
    address_opt recipient_owner,  ///< Recipient internal owner (contract address).
    uint128     tokens,           ///< Amount of tokens to transfer.
    uint128     evers,            ///< Native funds to process.
                                  ///< For internal requests, this value is ignored
                                  ///<  and processing costs will be taken from attached value.
    bool        deploy,           ///< Contract will send internalTransfer message with StateInit
                                  ///<  to also deploy new tip3 wallet (if it doesn't already exist) with
                                  ///<  the provided recipient_public_key and recipient_internal_owner.
    uint128     return_ownership, ///< Return ownership - to decrease lend ownership for the caller contract (additionally).
    cell        payload           ///< Payload (arbitrary cell) - will be transmitted into owner's notification.
  ) = 13;
#endif // TIP3_DEPLOY_TRANSFER

  /// Request wallet token balance using internal message (contract-to-contract).
  [[internal, noaccept, answer_id]]
  uint128 requestBalance() = 14;

  /// Receive tokens from root (RootTokenContract).
  [[internal, noaccept, answer_id]]
  bool accept(
    uint128 tokens,       ///< Tokens received from RootTokenContract.
    address answer_addr,  ///< Answer address.
    uint128 keep_evers    ///< Native funds that the wallet should keep before returning answer with the remaining funds.
  ) = 15;

  /// Receive tokens from another tip3 wallet.
  [[internal, noaccept, answer_id]]
  void internalTransfer(
    uint128     tokens,          ///< Amount of tokens received from another tip3 token wallet.
    address     answer_addr,     ///< Answer address.
    uint256     sender_pubkey,   ///< Sender wallet pubkey.
    address_opt sender_owner,    ///< Sender wallet internal owner.
    bool        notify_receiver, ///< If notify_receiver is specified,
                                 ///<  the wallet should send notification to its internal owner.
    cell        payload          ///< Payload (arbitrary cell).
  ) = 16;

#ifdef TIP3_ENABLE_DESTROY
  /// Send the remaining !native! funds to \p dest and destroy the wallet.
  /// Token balance must be zero. Not allowed for lend ownership.
  TIP3_EXTERNAL
  [[internal, noaccept]]
  void destroy(
    address dest ///< Where to send the remaining evers.
  ) = 17;
#endif // TIP3_ENABLE_DESTROY

#ifdef TIP3_ENABLE_BURN
  /// The wallet will send self destruct message transferring all the remaining funds to the Wrapper.
  /// Wrapper will transfer the same amount of external (wrapped) tip3 tokens from his wallet to a wallet with
  ///  { out_pubkey, out_owner } ownership.
  /// Not allowed for lend ownership.
  [[internal, noaccept, answer_id]]
  void burn(
    uint256     out_pubkey, ///< Pubkey of external (wrapped) tip3 wallet.
                            ///< Where to return external tip3 tokens.
    address_opt out_owner   ///< Internal owner (contract) of external (wrapped) tip3 wallet.
                            ///< Where to return external tip3 tokens.
  ) = 18;
#endif // TIP3_ENABLE_BURN

#ifdef TIP3_ENABLE_LEND_OWNERSHIP
  /// Lend ownership to some contract until 'lend_finish_time' for the specified amount of tokens.
  /// Allowance is required to be empty and is not permited to set up by temporary owner.
  /// Will send ITONTokenWalletNotify::onTip3LendOwnership() notification to \p dest.
  FLEX_EXTERNAL
  [[internal, noaccept, answer_id]]
  void lendOwnership(
    address answer_addr,      ///< Answer address.
    uint128 evers,            ///< Native funds to process.
                              ///< For internal requests, this value is ignored
                              ///<  and processing costs will be taken from attached value.
    address dest,             ///< Destination address.
    uint128 lend_balance,     ///< Amount of tokens to lend ownership.
    uint32  lend_finish_time, ///< Lend ownership finish time.
    cell    deploy_init_cl,   ///< Serialized StateInit struct to send a deploy message
                              ///<  with onTip3LendOwnership notification.
                              ///< Should be an empty cell or StateInit with empty code or data
                              ///<  to send non-deploy message.
    cell    payload           ///< Payload (arbitrary cell). Will be transmitted into onTip3LendOwnership notification.
  ) = 19;

  TIP3_EXTERNAL
  [[internal, noaccept]]
  void lendOwnershipPubkey(
    opt<uint256> pubkey,          ///< Lend owner public key. Revoke if empty.
    opt<uint32>  lend_finish_time ///< Lend ownership finish time. If empty, will be taken from the current lend pubkey record.
  ) = 20;

  /// Return ownership back to the original owner (for the provided amount of tokens).
  [[internal, noaccept]]
  void returnOwnership(
    uint128 tokens ///< Amount of tokens to return.
  ) = 21;
#endif // TIP3_ENABLE_LEND_OWNERSHIP

  // =============================== getters =============================== //
  /// Get info about contract state details.
  [[getter]]
  details_info getDetails() = 22;

#ifdef TIP3_ENABLE_EXTERNAL
  /// Get contract token balance.
  [[getter]]
  uint128 getBalance() = 23;
#endif // TIP3_ENABLE_EXTERNAL

#ifdef TIP3_ENABLE_ALLOWANCE
  // ========================= allowance interface ========================= //
  /// Approve allowance for some spender.
  TIP3_EXTERNAL
  [[internal, noaccept]]
  void approve(
    address spender,         ///< Target tip3 wallet.
    uint128 remainingTokens, ///< Currently remaining tokens in allowance.
    uint128 tokens           ///< Amount of tokens to be set up in allowance.
  ) = 24;

  /// Request transfer by allowance from another tip3 wallet \p from to tip3 wallet \p to.
  TIP3_EXTERNAL
  [[internal, noaccept]]
  void transferFrom(
    address answer_addr, ///< Answer address.
    address from,        ///< Address of tip3 wallet where allowance is set up to this wallet.
    address to,          ///< Destination tip3 wallet address.
    uint128 tokens,      ///< Amount of tokens to transfer.
    uint128 evers        ///< Native funds to process.
                         ///< For internal requests, this value is ignored
                         ///<  and processing costs will be taken from attached value.
  ) = 25;

  /// "WithNotify" version of transferFrom.
  /// Request transfer by allowance from another tip3 wallet \p from to tip3 wallet \p to
  ///  with notification to destination contract's internal owner.
  TIP3_EXTERNAL
  [[internal, noaccept]]
  void transferFromWithNotify(
    address answer_addr, ///< Answer address.
    address from,        ///< Address of tip3 wallet where allowance is set up to this wallet.
    address to,          ///< Destination tip3 wallet address.
    uint128 tokens,      ///< Amount of tokens to transfer.
    uint128 evers,       ///< Native funds to process.
    cell    payload      ///< Payload (arbitrary cell). Will be transmitted into onTip3Transfer notification.
  ) = 26;

  /// Request of allowance transfer from spender tip3 wallet
  [[internal]]
  void internalTransferFrom(
    address answer_addr,     ///< Answer address
    address to,              ///< Destination tip3 wallet address.
    uint128 tokens,          ///< Amount of tokens to transfer.
    bool    notify_receiver, ///< Send onTip3Transfer notification to target's owner contract (if set up)
    cell    payload          ///< Payload (arbitrary cell). Will be transmitted into onTip3Transfer notification.
  ) = 27;

  /// Disapprove allowance
  TIP3_EXTERNAL
  [[internal, noaccept]]
  void disapprove() = 28;
#endif // TIP3_ENABLE_ALLOWANCE
};
using ITONTokenWalletPtr = handle<ITONTokenWallet>;

/// TONTokenWallet persistent data struct
struct DTONTokenWallet {
  string      name_;              ///< Token name.
  string      symbol_;            ///< Token short symbol.
  uint8       decimals_;          ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  uint128     balance_;           ///< Token balance of the wallet.
  uint256     root_pubkey_;       ///< Public key of the related RootTokenContract.
  address     root_address_;      ///< Address of the related RootTokenContract.
  uint256     wallet_pubkey_;     ///< Public key of wallet owner.
  address_opt owner_address_;     ///< Owner contract address for internal ownership.
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
  opt<lend_pubkey> lend_pubkey_;  ///< Lend ownership pubkey.
  lend_owners_map  lend_owners_;  ///< Lend ownership map ({service owner, user_id} => lend_owner).
#endif // TIP3_ENABLE_LEND_OWNERSHIP
#ifdef TIP3_DEPLOY_TRANSFER
  cell code_;                     ///< Until MYCODE support, we need code for self deploying.
#endif // TIP3_DEPLOY_TRANSFER
  uint256 code_hash_;             ///< Tip3 wallet code hash to verify other wallets.
  uint16  code_depth_;            ///< Tip3 wallet code depth to verify other wallets.
#ifdef TIP3_ENABLE_ALLOWANCE
  opt<allowance_info> allowance_; ///< Allowance.
#endif // TIP3_ENABLE_ALLOWANCE
  int8 workchain_id_;             ///< Workchain id.
};

// TODO: implement filter reflection instead of code duplication
// struct [[disable("lend_ownership"), enable("allowance")]] ExternalCfg {};
// struct [[enable("lend_ownership"), disable("allowance")]] InternalCfg {};
// using DTONTokenWalletExternal = __reflect_filter<DTONTokenWallet, ExternalCfg>;
// using DTONTokenWalletInternal = __reflect_filter<DTONTokenWallet, InternalCfg>;
// using DTONTokenWalletExternal = __reflect_filter<DTONTokenWallet, [[disable("lend_ownership"), enable("allowance")]]>;

struct DTONTokenWalletExternal {
  string      name_;              ///< Token name.
  string      symbol_;            ///< Token short symbol.
  uint8       decimals_;          ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  uint128     balance_;           ///< Token balance of the wallet.
  uint256     root_pubkey_;       ///< Public key of the related RootTokenContract.
  address     root_address_;      ///< Address of the related RootTokenContract.
  uint256     wallet_pubkey_;     ///< Public key of wallet owner.
  address_opt owner_address_;     ///< Owner contract address for internal ownership.
  cell        code_;              ///< Until MYCODE support, we need code for self deploying.
  uint256     code_hash_;         ///< Tip3 wallet code hash to verify other wallets.
  uint16      code_depth_;        ///< Tip3 wallet code depth to verify other wallets.
  opt<allowance_info> allowance_; ///< Allowance.
  int8        workchain_id_;      ///< Workchain id.
};

struct DTONTokenWalletInternal {
  string      name_;                 ///< Token name.
  string      symbol_;               ///< Token short symbol.
  uint8       decimals_;             ///< Decimals for ui purposes. ex: balance 100 with decimals 2 will be printed as 1.00.
  uint128     balance_;              ///< Token balance of the wallet.
  uint256     root_pubkey_;          ///< Public key of the related RootTokenContract.
  address     root_address_;         ///< Address of the related RootTokenContract.
  uint256     wallet_pubkey_;        ///< Public key of wallet owner.
  address_opt owner_address_;        ///< Owner contract address for internal ownership.
  opt<lend_pubkey>  lend_pubkey_;    ///< Lend ownership pubkeys (pubkey => lend_pubkey).
  lend_owners_map   lend_ownership_; ///< Lend ownership map ({service owner, user_id} => lend_owner).
  uint256           code_hash_;      ///< Tip3 wallet code hash to verify other wallets.
  uint16            code_depth_;     ///< Tip3 wallet code depth to verify other wallets.
  int8              workchain_id_;   ///< Workchain id.
};

/// \interface ETONTokenWallet
/// \brief TONTokenWallet events interface
struct ETONTokenWallet {
};

/// Prepare TONTokenWallet persistent data struct
inline
DTONTokenWallet prepare_wallet_data(
  string name, string symbol, uint8 decimals,
  uint256 root_pubkey, address root_address,
  uint256 wallet_pubkey, address_opt wallet_owner,
  uint256 code_hash, uint16 code_depth, int8 workchain_id, cell code
) {
  return {
    name, symbol, decimals,
    uint128(0), root_pubkey, root_address,
    wallet_pubkey, wallet_owner,
#ifdef TIP3_ENABLE_LEND_OWNERSHIP
    {}, {},
#endif
#ifdef TIP3_DEPLOY_TRANSFER
    code,
#endif
    code_hash, code_depth,
#ifdef TIP3_ENABLE_ALLOWANCE
    {},
#endif
    workchain_id
  };
}

/// Calculate wallet original StateInit hash (to get its deploy address).
/// This version depends on macroses configuration (external / internal wallet).
__always_inline
uint256 calc_wallet_init_hash(
  string name, string symbol, uint8 decimals,
  uint256 root_pubkey, address root_address, uint256 wallet_pubkey, address_opt wallet_owner,
  uint256 code_hash, uint16 code_depth, int8 workchain_id, cell code
) {
  DTONTokenWallet wallet_data =
    prepare_wallet_data(name, symbol, decimals, root_pubkey, root_address, wallet_pubkey, wallet_owner,
                        code_hash, code_depth, workchain_id, code);
  auto init_hdr = persistent_data_header<ITONTokenWallet, wallet_replay_protection_t>::init();
  cell data_cl = prepare_persistent_data<ITONTokenWallet, wallet_replay_protection_t>(init_hdr, wallet_data);
  return tvm_state_init_hash(code_hash, uint256(tvm_hash(data_cl)), code_depth, uint16(data_cl.cdepth()));
}

/// Calculate wallet original StateInit hash (to get its deploy address).
/// For external wallets.
__always_inline
uint256 calc_ext_wallet_init_hash(
  string name, string symbol, uint8 decimals,
  uint256 root_pubkey, address root_address, uint256 wallet_pubkey, address_opt wallet_owner,
  uint256 code_hash, uint16 code_depth, int8 workchain_id, cell code
) {
  DTONTokenWalletExternal wallet_data {
    name, symbol, decimals,
    uint128(0), root_pubkey, root_address, wallet_pubkey, wallet_owner,
    code, code_hash, code_depth, {}, workchain_id
  };
  auto init_hdr = persistent_data_header<ITONTokenWallet, wallet_replay_protection_t>::init();
  cell data_cl = prepare_persistent_data<ITONTokenWallet, wallet_replay_protection_t>(init_hdr, wallet_data);
  return tvm_state_init_hash(code_hash, uint256(tvm_hash(data_cl)), code_depth, uint16(data_cl.cdepth()));
}

/// Calculate wallet original StateInit hash (to get its deploy address).
/// For internal (flex) wallets.
__always_inline
uint256 calc_int_wallet_init_hash(
  string name, string symbol, uint8 decimals,
  uint256 root_pubkey, address root_address, uint256 wallet_pubkey, address_opt wallet_owner,
  uint256 code_hash, uint16 code_depth, int8 workchain_id
) {
  DTONTokenWalletInternal wallet_data {
    name, symbol, decimals,
    uint128(0), root_pubkey, root_address, wallet_pubkey, wallet_owner,
    {}, {}, code_hash, code_depth, workchain_id
  };
  auto init_hdr = persistent_data_header<ITONTokenWallet, wallet_replay_protection_t>::init();
  cell data_cl = prepare_persistent_data<ITONTokenWallet, wallet_replay_protection_t>(init_hdr, wallet_data);
  return tvm_state_init_hash(code_hash, uint256(tvm_hash(data_cl)), code_depth, uint16(data_cl.cdepth()));
}

/// Prepare Token Wallet StateInit structure and expected contract address (hash from StateInit).
/// This version depends on macroses configuration (external / internal wallet).
inline
std::pair<StateInit, uint256> prepare_wallet_state_init_and_addr(DTONTokenWallet wallet_data, cell code) {
  auto init_hdr = persistent_data_header<ITONTokenWallet, wallet_replay_protection_t>::init();
  cell wallet_data_cl =
    prepare_persistent_data<ITONTokenWallet, wallet_replay_protection_t>(init_hdr, wallet_data);
  StateInit wallet_init {
    /*split_depth*/{}, /*special*/{},
    code, wallet_data_cl, /*library*/{}
  };
  cell wallet_init_cl = build(wallet_init).make_cell();
  return { wallet_init, uint256(tvm_hash(wallet_init_cl)) };
}

/// Prepare Token Wallet StateInit structure and expected contract address (hash from StateInit).
/// For external wallets.
inline
std::pair<StateInit, uint256> prepare_external_wallet_state_init_and_addr(
  string name, string symbol, uint8 decimals,
  uint256 root_pubkey, address root_address,
  uint256 wallet_pubkey, address_opt wallet_owner,
  uint256 code_hash, uint16 code_depth,
  int8 workchain_id, cell code
) {
  DTONTokenWalletExternal wallet_data {
    name, symbol, decimals,
    uint128(0), root_pubkey, root_address, wallet_pubkey, wallet_owner,
    code, code_hash, code_depth, {}, workchain_id
  };
  cell wallet_data_cl =
    prepare_persistent_data<ITONTokenWallet, wallet_replay_protection_t, DTONTokenWalletExternal>(
      wallet_replay_protection_t::init(), wallet_data);
  StateInit wallet_init {
    /*split_depth*/{}, /*special*/{},
    code, wallet_data_cl, /*library*/{}
  };
  cell wallet_init_cl = build(wallet_init).make_cell();
  return { wallet_init, uint256(tvm_hash(wallet_init_cl)) };
}

/// Prepare Token Wallet StateInit structure and expected contract address (hash from StateInit).
/// For internal wallets.
inline
std::pair<StateInit, uint256> prepare_internal_wallet_state_init_and_addr(
  string name, string symbol, uint8 decimals,
  uint256 root_pubkey, address root_address,
  uint256 wallet_pubkey, address_opt wallet_owner,
  uint256 code_hash, uint16 code_depth,
  int8 workchain_id, cell code
) {
  DTONTokenWalletInternal wallet_data {
    name, symbol, decimals,
    uint128(0), root_pubkey, root_address, wallet_pubkey, wallet_owner,
    {}, {}, code_hash, code_depth, workchain_id
  };
  cell wallet_data_cl =
    prepare_persistent_data<ITONTokenWallet, wallet_replay_protection_t, DTONTokenWalletInternal>(
      wallet_replay_protection_t::init(), wallet_data
      );
  StateInit wallet_init {
    /*split_depth*/{}, /*special*/{},
    code, wallet_data_cl, /*library*/{}
  };
  cell wallet_init_cl = build(wallet_init).make_cell();
  return { wallet_init, uint256(tvm_hash(wallet_init_cl)) };
}

} // namespace tvm

