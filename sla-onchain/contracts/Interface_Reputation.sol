// SPDX-License-Identifier: MIT
pragma solidity ^0.8.17;

interface Interface_Reputation {

    /**
     * Notify the Reputation contract that a checkpoint report has been submitted
     * @param providerHardwareId The hardware id of the provider's enclave
     * @param clientAddr  The address of the client
     * @param paymentAmount The amount that the provider got paid
     */
    function onCheckpointReport(
        bytes32 providerHardwareId,
        address clientAddr,
        uint256 paymentAmount
    ) external;

    /**
     * Notify the Reputation contract that a dispute has been resolved
     * @param providerHardwareId The hardware id of the provider's enclave
     * @param clientAddr  The address of the client
     */
    function onResolvedDispute(
        bytes32 providerHardwareId,
        address clientAddr
    ) external;

    /**
     * Notify the Reputation contract of unresolved disputes
     * @param providerHardwareId The hardware id of the provider's enclave
     * @param clientAddr  The address of the client
     * @param numUnresolvedDisputes The number of unresolved disputes
     */
    function onUnresolvedDispute(
        bytes32 providerHardwareId,
        address clientAddr,
        uint64  numUnresolvedDisputes
    ) external;
}