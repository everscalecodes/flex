/** \file
 *  \brief Exchange pair (tip3/tip3) contract interfaces and data-structs.
 *  XchgPair - contract defining tip3/tip3 exchange pair. May only be deployed by Flex root contract.
 *  \author Andrew Zhogin
 *  \copyright 2019-2022 (c) TON LABS
 */

#pragma once

#include "FlexWallet.hpp"
#include "XchgPairSalt.hpp"

#include <tvm/schema/message.hpp>

#include <tvm/smart_switcher.hpp>
#include <tvm/contract_handle.hpp>

namespace tvm {

/** \interface IXchgPair
 *  \brief XchgPair contract interface.
 */
__interface IXchgPair {

  /// Initialization method, may only be called by Flex root.
  [[internal, noaccept, answer_id, deploy]]
  bool onDeploy(
    uint128 min_amount,           ///< Minimum amount of major tokens for a deal or an order
    uint128 deploy_value,         ///< Crystals to be kept in the contract
    address notify_addr,          ///< Notification address (AMM)
    Tip3Config major_tip3cfg,     ///< Major tip3 configuration
    Tip3Config minor_tip3cfg      ///< Minor tip3 configuration
  ) = 10;

  // ========== getters ==========
  /// Get address of Flex root
  [[getter]]
  address getFlexAddr() = 11;

  /// Get address of major tip3 token root
  [[getter]]
  address getTip3MajorRoot() = 12;

  /// Get address of minor tip3 token root
  [[getter]]
  address getTip3MinorRoot() = 13;

  /// Get minimum amount of major tokens for a deal or an order
  [[getter]]
  uint128 getMinAmount() = 14;

  /// Get notification address (AMM)
  [[getter]]
  address getNotifyAddr() = 15;

  /// Get major reserve wallet
  [[getter]]
  address getMajorReserveWallet() = 16;

  /// Get minor reserve wallet
  [[getter]]
  address getMinorReserveWallet() = 17;

  /// Get PriceXchg contract code with configuration salt added.
  [[getter]]
  cell getXchgPriceCode() = 18;

  /// Get XchgPair configuration from code salt (common for all pairs of one flex).
  [[getter]]
  XchgPairSalt getConfig() = 19;
};
using IXchgPairPtr = handle<IXchgPair>;

/// XchgPair persistent data struct
struct DXchgPair {
  address tip3_major_root_;       ///< Address of RootTokenContract for major tip3 token
  address tip3_minor_root_;       ///< Address of RootTokenContract for minor tip3 token
  uint128 min_amount_;            ///< Minimum amount of major tokens for a deal or an order
  address notify_addr_;           ///< Notification address (AMM)
  address major_reserve_wallet_;  ///< Major reserve wallet
  address minor_reserve_wallet_;  ///< Minor reserve wallet
  opt<Tip3Config> major_tip3cfg_; ///< Configuration of the major tip3 token.
  opt<Tip3Config> minor_tip3cfg_; ///< Configuration of the minor tip3 token.
};

/// \interface EXchgPair
/// \brief XchgPair events interface
__interface EXchgPair {
};

/// Prepare Exchange Pair StateInit structure and expected contract address (hash from StateInit)
inline
std::pair<StateInit, uint256> prepare_xchg_pair_state_init_and_addr(DXchgPair pair_data, cell pair_code) {
  cell pair_data_cl = prepare_persistent_data<IXchgPair, void, DXchgPair>({}, pair_data);
  StateInit pair_init {
    /*split_depth*/{}, /*special*/{},
    pair_code, pair_data_cl, /*library*/{}
  };
  cell pair_init_cl = build(pair_init).make_cell();
  return { pair_init, uint256(tvm_hash(pair_init_cl)) };
}

} // namespace tvm

