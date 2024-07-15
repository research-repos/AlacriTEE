// SPDX-License-Identifier: MIT
pragma solidity ^0.8.17;

import {BytesUtils} from "../libs/ra-onchain/libs/ens-contracts/BytesUtils.sol";
import {LibSecp256k1Sha256} from "../libs/ra-onchain/contracts/LibSecp256k1Sha256.sol";

import {Interface_Reputation} from "./Interface_Reputation.sol";
import {Interface_SlaManager} from "./Interface_SlaManager.sol";

contract SLA {
    using BytesUtils for bytes;

    //===== structs =====

    struct Provider {
        uint256 balanceWei;
        address appKeyAddr;
        address walletAddr;
        bytes32 hardwareId;
    }

    struct Client {
        uint256 balanceWei;
        address walletAddr;
    }

    struct Dispute {
        bool submitted;
        uint256 payment;
        uint256 startBlock;
        bool isEncryptedResReceived;
        bool settled;
    }

    //===== Member variables =====

    Provider m_provider;
    Client m_client;

    address m_reputationAddr;
    address m_slaManagerAddr;
    bool    m_isActive;
    uint64  m_numDisputes;
    uint256 m_pricePerUnit;
    uint256 m_closingBlockNum;
    uint256 m_contractId;
    uint64  m_currChkptSeqNum;
    uint256 m_currReqId;

    // contract close period in number of blocks. 0 for testing.
    uint32  constant CONTRACT_CLOSE_PERIOD = 0;

    uint32  constant DISPUTE_RESPONSE_PERIOD = 2;   // in number of blocks
    uint256 constant DISPUTE_RESPONSE_COST = 90000;

    uint8 constant CHKPT_REPORT_ENTRY_SIZE = 8;

    mapping (uint256 => Dispute) m_disputes;

    mapping (uint64 => uint64) m_chkptAvgUnitUsed;
    mapping (uint64 => uint256) m_chkptReqIdEnd;

    //===== Events =====

    event CheckpointReportSubmitted(
        address indexed slaAddr,
        uint256 checkpointSeqNum,
        uint256 numRequests,
        uint256 numAccepted,
        uint256 providerBalance,
        uint256 clientBalance
    );
    event SlaContractCreated(address indexed slaAddr, address, address clientWalletAddr);

    // disputes
    event ClientDispute(address indexed slaAddr, uint256 reqId, bool isEncryptedResReceived);

    event ResolvedDisputeWithACK(
        address indexed slaAddr,
        uint256 reqId
    );

    event ResolvedDisputeWithKey(
        address indexed slaAddr,
        uint256 reqId,
        bytes encryptedDecryptionKey
    );

    event ContractCloseInitiated(address indexed slaAddr);


    //===== Constructor =====

    /**
     *
       params)
     */
    constructor(
        address reputationAddr,
        bytes32 providerHardwareId,
        address providerAppKeyAddr,
        address providerWalletAddr,
        address clientWalletAddr,
        uint256 providerBalanceWei,
        uint256 clientBalanceWei,
        uint256 pricePerUnit,
        uint256 contractId
    )
        payable
    {
        // assign sla manager address
        m_slaManagerAddr = msg.sender;

        // assign reputation address
        m_reputationAddr = reputationAddr;

        // assign contract id
        m_contractId = contractId;

        // instantiate provider
        m_provider = Provider({
            balanceWei: providerBalanceWei,
            appKeyAddr: providerAppKeyAddr,
            walletAddr: providerWalletAddr,
            hardwareId: providerHardwareId
        });

        // instantiate client
        m_client = Client({
            balanceWei: clientBalanceWei,
            walletAddr: clientWalletAddr
        });

        // set price per unit
        m_pricePerUnit = pricePerUnit;

        // set num disputes
        m_numDisputes = 0;

        // set contract to active
        m_isActive = true;

        // set current checkpoint seq num
        m_currChkptSeqNum = 0;

        // set current request id
        m_currReqId = 0;

        // emit event
        emit SlaContractCreated(address(this), providerWalletAddr, clientWalletAddr);
    }

    //===== internal Functions =====

    /**
     * Reimburse the provider and settle the dispute
     * @param disputeEntry The dispute entry
     */
    function reimburseAndSettle(
        Dispute storage disputeEntry
    ) internal {
        // reimburse provider
        m_provider.balanceWei += disputeEntry.payment / 2;

        // deduct from client balance
        if (disputeEntry.payment / 2 > m_client.balanceWei) {
            m_client.balanceWei = 0;
        } else {
            m_client.balanceWei -= disputeEntry.payment / 2;
        }

        // resolve dispute
        disputeEntry.settled = true;

        // decrement number of disputes
        m_numDisputes--;

        // notify reputation contract
        Interface_Reputation(m_reputationAddr).onResolvedDispute(
            m_provider.hardwareId,
            m_client.walletAddr
        );
    }

    //===== external Functions =====

    /**
     * Submit a checkpoint report
     * @param chkptSeqNum    The checkpoint sequence number
     * @param numRequests    The number of requests in the checkpoint
     * @param checkpointData The checkpoint data
     */
    function hostCheckpointReport(
        uint64 chkptSeqNum,
        uint64 numRequests,
        bytes memory checkpointData
    )
        external
    {
        // 1. make sure the checkpoint seq num is correct
        require(
            chkptSeqNum == m_currChkptSeqNum,
            "Checkpoint seq num is incorrect"
        );
        m_currChkptSeqNum++;

        // 2. verify that the message is signed by the provider
        require(
            m_provider.appKeyAddr == msg.sender,
            "Message signature invalid"
        );

        // 3. each entry in report needs `CHKPT_REPORT_ENTRY_SIZE` bytes
        //    thus, the length of the report should be numRequests * CHKPT_REPORT_ENTRY_SIZE
        require(
            checkpointData.length == numRequests * CHKPT_REPORT_ENTRY_SIZE,
            "Invalid checkpoint data"
        );

        // 4. parse the checkpoint data
        uint256 numAccepted = 0;
        uint256 totalUnitsUsed = 0;
        for (uint64 i = 0; i < numRequests; i++) {
            uint64 unitsUsed = 0;

            unitsUsed = checkpointData.readUint64(
                i * CHKPT_REPORT_ENTRY_SIZE
            );
            totalUnitsUsed += unitsUsed;
            if (unitsUsed > 0)
            {
                numAccepted++;
            }
        }

        // we assume the enclave correctly orders the requests by reqId
        m_currReqId += numRequests;

        // calculate the average units used for this checkpoint
        m_chkptAvgUnitUsed[chkptSeqNum] = uint64(totalUnitsUsed / numRequests);
        m_chkptReqIdEnd[chkptSeqNum] = m_currReqId;

        // calculate payment to provider
        uint256 totalPayment = totalUnitsUsed * m_pricePerUnit;

        // check that the client has enough balance to pay
        require(
            m_client.balanceWei >= totalPayment,
            "Client does not have enough balance"
        );
        m_client.balanceWei -= totalPayment;
        // 5. update provider and client balances
        m_provider.balanceWei += totalPayment;


        // 6. notify reputation contract
        Interface_Reputation(m_reputationAddr).onCheckpointReport(
            m_provider.hardwareId,
            m_client.walletAddr,
            totalPayment
        );

        // 7. update provider checkpoint seq num
        emit CheckpointReportSubmitted(
            address(this),
            chkptSeqNum,
            numRequests,
            numAccepted,
            m_provider.balanceWei,
            m_client.balanceWei
        );
    }

    /**
     * Client sends a dispute
     * @param reqId The request id
     * @param isEncryptedResReceived Whether the client has received the encrypted response
     */
    function dispute(
        uint256 reqId,
        uint64 chkptSeqNum,
        bool isEncryptedResReceived
    ) external
    {
        // require that the contract is active
        require(
            m_isActive == true,
            "Contract is not active"
        );

        // check that the sender is the client
        require(
            msg.sender == m_client.walletAddr,
            "Only client can dispute"
        );

        // check that the request id is valid
        require(
            m_currReqId > reqId,
            "Invalid request id"
        );

        // check that the checkpoint seq num is valid
        require(
            (
                // check that the checkpoint seq num is less than the current checkpoint seq num
                (chkptSeqNum < m_currChkptSeqNum) &&
                // check that the request id is under the reqId end of the checkpoint seq num
                (reqId < m_chkptReqIdEnd[chkptSeqNum]) &&
                // check that the request id is greater than the reqId end of the previous checkpoint seq num
                (
                    (chkptSeqNum == 0) ||
                    (m_chkptReqIdEnd[chkptSeqNum - 1] <= reqId)
                )
            ),
            "Invalid checkpoint seq num"
        );

        Dispute storage disputeEntry = m_disputes[reqId];

        // check that the dispute does not already exist
        require(
            disputeEntry.submitted == false,
            "Dispute already submitted"
        );

        // check that the dispute has not already been resolved
        require(
            disputeEntry.settled == false,
            "Dispute has already been resolved"
        );

        disputeEntry.submitted = true;

        uint64 unitsUsed = m_chkptAvgUnitUsed[chkptSeqNum];

        // revert back half of the payment for the disputed request
        disputeEntry.payment = unitsUsed * m_pricePerUnit;
        // check that provider has enough balance to reimburse
        require(
            m_provider.balanceWei >= disputeEntry.payment / 2,
            "Provider does not have enough balance"
        );
        m_provider.balanceWei -= disputeEntry.payment / 2;
        m_client.balanceWei += disputeEntry.payment / 2;

        // assign variable for encrypted response received
        disputeEntry.isEncryptedResReceived = isEncryptedResReceived;

        // set the dispute starting block num
        disputeEntry.startBlock = block.number;

        // increment the number of disputes
        m_numDisputes++;

        // emit event
        emit ClientDispute(address(this), reqId, isEncryptedResReceived);
    }


    /**
     * provider sends a dispute response indicating that the corresponding
     * request was not accepted
     *
     */
    function disputeResponseWithRejectedReq(
        uint256 reqId
    )
        external
    {
        // verify the message is from the provider
        require(
            m_provider.appKeyAddr == msg.sender,
            "Message signature invalid"
        );

        Dispute storage disputeEntry = m_disputes[reqId];

        // check that the dispute submitted
        require(
            disputeEntry.submitted == true,
            "Dispute does not exist"
        );

        // check that the dispute has not already been resolved
        require(
            disputeEntry.settled == false,
            "Dispute has already been resolved"
        );

        // equire that the dispute be sent within the dispute response period
        require(
            block.number <= disputeEntry.startBlock + DISPUTE_RESPONSE_PERIOD,
            "Dispute response period has passed"
        );

        reimburseAndSettle(disputeEntry);
    }

    /**
     * provider sends a dispute response with the client's ACK
     *
     */
    function disputeResponseWithAck(
        uint256 reqId,
        bytes32 r,
        bytes32 s
    )
        external
    {
        // verify the message is from the provider
        require(
            m_provider.appKeyAddr == msg.sender,
            "Message signature invalid"
        );

        Dispute storage disputeEntry = m_disputes[reqId];

        // check that the dispute submitted
        require(
            disputeEntry.submitted == true,
            "Dispute does not exist"
        );

        // check that the dispute has not already been resolved
        require(
            disputeEntry.settled == false,
            "Dispute has already been resolved"
        );

        // equire that the dispute be sent within the dispute response period
        require(
            block.number <= disputeEntry.startBlock + DISPUTE_RESPONSE_PERIOD,
            "Dispute response period has passed"
        );

        // check that the dispute has ResolvedDisputeWithKey set to false
        require(
            disputeEntry.isEncryptedResReceived == false,
            "Incorrect response type"
        );

        // verify signature of the ACK message
        bytes memory ackMessage = abi.encode(reqId, bytes32('ACK'));

        require(
            LibSecp256k1Sha256.verifySignMsg(
                m_client.walletAddr,
                ackMessage,
                r,
                s
            ),
            "ACK message signature invalid"
        );

        // emit event
        emit ResolvedDisputeWithACK(address(this), reqId);

        // reimburse and settle
        reimburseAndSettle(disputeEntry);
    }

    /**
     * Provider sends a dispute response
     */
    function disputeResponseWithKey(
        uint256 reqId,
        bytes memory encryptedDecryptionKey
    )
        external
    {
        // verify signature from provider
        require(
            m_provider.appKeyAddr == msg.sender,
            "Message signature invalid"
        );

        Dispute storage disputeEntry = m_disputes[reqId];

        // check that the dispute submitted
        require(
            disputeEntry.submitted == true,
            "Dispute does not exist"
        );

        // check that the dispute has not already been resolved
        require(
            disputeEntry.settled == false,
            "Dispute has already been resolved"
        );

        // equire that the dispute be sent within the dispute response period
        require(
            block.number <= disputeEntry.startBlock + DISPUTE_RESPONSE_PERIOD,
            "Dispute response period has passed"
        );

        // check that the dispute has ResolvedDisputeWithKey set to true
        require(
            disputeEntry.isEncryptedResReceived == true,
            "Incorrect response type"
        );

        // 3. emit event containing key
        emit ResolvedDisputeWithKey(
            address(this),
            reqId,
            encryptedDecryptionKey
        );

        // 4. reimburse and settle
        reimburseAndSettle(disputeEntry);

    } // end disputeResponse

    /**
     * Close the contract. This sets a period of time for the host to send any
       unsent checkpoint reports and for the client to send any disputes for
       responses that it hasn't received yet.
     */
    function closeContract() external
    {
        // require that the contract is active
        require(
            m_isActive == true,
            "Contract is not active"
        );

        // require that the closing party is either the client or the provider
        require(
            msg.sender == m_client.walletAddr || msg.sender == m_provider.walletAddr,
            "Only client or provider can close contract"
        );

        // set the closing block num
        m_closingBlockNum = block.number;

        // emit event
        emit ContractCloseInitiated(address(this));
    }

    /**
     * Finish closing the contract. This function can only be called after the
       closing period has passed.
     */
    function finishClosingContract() external
    {
        // require that the contract is active
        require(
            m_isActive == true,
            "Contract is not active"
        );

        // require that the contract close period has passed
        require(
            block.number > m_closingBlockNum + CONTRACT_CLOSE_PERIOD,
            "Contract close period has not passed"
        );

        // call finalize function in SlaManager
        Interface_SlaManager(m_slaManagerAddr).finalizeSlaContract(
            m_contractId
        );

        // notify Reputation of any unresolved disputes
        if (m_numDisputes > 0) {
            Interface_Reputation(m_reputationAddr).onUnresolvedDispute(
                m_provider.hardwareId,
                m_client.walletAddr,
                m_numDisputes
            );
        }

        // set the contract to inactive
        m_isActive = false;
    }

    function getProviderBalance() external view returns (uint256) {
        return m_provider.balanceWei;
    }

    function getClientBalance() external view returns (uint256) {
        return m_client.balanceWei;
    }

} // end contract SLA
