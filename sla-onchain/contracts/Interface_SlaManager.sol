// SPDX-License-Identifier: MIT
pragma solidity ^0.8.17;

interface Interface_SlaManager {

    /**
     * Notify the SLA contract that a checkpoint report has been submitted
     * @param contractId The ID of the SLA contract
     */
    function finalizeSlaContract(
        uint256 contractId
    ) external payable;
}