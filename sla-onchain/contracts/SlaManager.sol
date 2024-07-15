// SPDX-License-Identifier: MIT
pragma solidity ^0.8.17;

import {DecentAppCert} from "../libs/ra-onchain/contracts/DecentAppCert.sol";
import {DecentCertChain} from "../libs/ra-onchain/contracts/DecentCertChain.sol";
import {Interface_DecentServerCertMgr} from "../libs/ra-onchain/contracts/Interface_DecentServerCertMgr.sol";
import {LibSecp256k1Sha256} from "../libs/ra-onchain/contracts/LibSecp256k1Sha256.sol";
import {Reputation} from "./Reputation.sol";
import {SLA} from "./SLA.sol";

contract SlaManager {
    //===== structs =====

    struct Provider {
        address appKeyAddress;
        address walletAddr;
        bool registered;
        uint256 balanceWei;
        uint256 rate; // rate in wei per compute unit
    }

    struct Client {
        address walletAddr;
        bool  registered;
        uint256 balanceWei;
        bytes32 dhKeyX;
        bytes32 dhKeyY;
    }

    struct ClientProposal {
        address clientWalletAddress;
        bytes32 providerHardwareId;
        bool    accepted;
        uint256 clientDeposit;
        uint256 providerDeposit;
    }

    struct SLAContract {
        address slaAddr;
        bool isActive;
    }

    //===== Member variables =====

    /**
     * @dev The address here is the address from the provider's private key
     * in the app credentials. This is used to verify the signature from the
       provider when it submits a checkpoint report
     */
    mapping(bytes32 => Provider) m_providerMap;

    /**
     * @dev The address here is associated to the client's Ethereum account
     */
    mapping(address => Client) m_clientMap;

    mapping(bytes32 => bool) m_revokedProvidersMap;
    mapping(uint256 => SLAContract) m_slaContractMap;
    mapping(uint256 => ClientProposal) m_clientProposalMap;

    address m_reputationAddr;
    address m_decentSvrCertMgrAddr;
    uint256  m_proposeCount = 0;
    uint256 m_minDepositWei = 100000000;

    //===== Events =====

    event ServiceDeployed(address indexed serviceAddr);
    event ProviderRegistered(
        bytes32 hardwareId,
        bytes32 dhKeyX,
        bytes32 dhKeyY,
        uint256 rate,
        bytes appCertDer
    );
    event SlaProposal(
        address clientAddr,
        bytes32 hardwareId,
        uint256 contractId,
        bytes32 cltDhKeyX,
        bytes32 cltDhKeyY
    );
    event SlaProposalAccepted(
        uint256 indexed contractId,
        address clientAddr,
        bytes32 hardwareId,
        address slaAddr,
        bytes providerMessage
    );
    event SlaContractFinalized(uint256 contractId);

    //===== Constructor =====

    constructor(address decentSvrCertMgr) {
        // 1. deploy the reputation contract
        Reputation reputationInst = new Reputation();
        m_reputationAddr = address(reputationInst);

        m_decentSvrCertMgrAddr = decentSvrCertMgr;

        // 2. emit event
        emit ServiceDeployed(address(this));
    }

    //===== external Functions =====

    /**
    * Get the address of the SLA contract given the contract ID
    */
    function getSlaAddress(uint256 contractId) external view returns (address) {
        return m_slaContractMap[contractId].slaAddr;
    }

    /**
     * Register a provider with the Desla service
     * @param rate The rate of the provider
     * @param svrCertDer The server certificate in DER format
     * @param appCertDer The application certificate in DER format
     */
    function registerProvider(
        uint64 rate,
        bytes32 dhKeyX,
        bytes32 dhKeyY,
        bytes memory svrCertDer,
        bytes memory appCertDer
    )
        external
    {
        // 1. verify the Decent certificate chain
        DecentAppCert.DecentApp memory appCert;
        DecentCertChain.verifyCertChain(
            appCert,
            m_decentSvrCertMgrAddr,
            svrCertDer,
            appCertDer
        );
        bytes32 hardwareId =
            Interface_DecentServerCertMgr(m_decentSvrCertMgrAddr).
                getPlatformId(appCert.issuerKeyAddr);

        Provider storage provider = m_providerMap[hardwareId];

        // make sure the provider has not already registered
        require(
            provider.registered == false,
            "Provider already registered"
        );

        // register the provider to the mapping
        provider.appKeyAddress = appCert.appKeyAddr;
        provider.walletAddr = tx.origin;
        provider.registered = true;
        provider.rate = rate;
        provider.balanceWei = 0;

        // 4. register the provider with the reputation contract
        Reputation(m_reputationAddr).registerProvider(hardwareId);

        // 5. emit event
        emit ProviderRegistered(hardwareId, dhKeyX, dhKeyY, rate, appCertDer);
    }

    /**
     * Register a client with the Desla service
     * @param dhKeyX The X coordinate of the client's public ECDH key
     * @param dhKeyY The Y coordinate of the client's public ECDH key
     */
    function registerClient(
        bytes32 dhKeyX,
        bytes32 dhKeyY
    )
        external
    {
        // 1. client address
        address clientWalletAddr = msg.sender;

        // 2. make sure the client has not already registered
        require(
            m_clientMap[clientWalletAddr].registered == false,
            "Client already registered"
        );

        // 3. update the mapping
        m_clientMap[clientWalletAddr] = Client({
            walletAddr: clientWalletAddr,
            registered: true,
            balanceWei: 0,
            dhKeyX: dhKeyX,
            dhKeyY: dhKeyY
        });

        // 4. register the client with the reputation contract
        Reputation(m_reputationAddr).registerClient(clientWalletAddr);
    }

    /**
     * Client proposes contract with a provider
     * @param hardwareId  The hardware ID of the provider enclave
     */
    function proposeContract(
        bytes32 hardwareId
    )
        external payable
        returns (uint256)
    {
        Client storage client = m_clientMap[msg.sender];

        // 1. make sure the client has already registered
        require(
            client.registered == true,
            "Client not registered"
        );

        // 2. make sure the provider has already registered
        require(
            m_providerMap[hardwareId].registered == true,
            "Provider not registered"
        );

        // 3. make sure the provider has not been revoked
        require(
            m_revokedProvidersMap[hardwareId] == false,
            "Provider revoked"
        );

        // 4. make sure the client has enough deposit
        require(
            msg.value >= m_minDepositWei,
            "Client deposit too low"
        );

        // 5. add client deposit to client's balance
        client.balanceWei += msg.value;

        // 6. create a proposal
        uint256 contractId = m_proposeCount++;

        // 7. update the mapping
        m_clientProposalMap[contractId] = ClientProposal({
            clientWalletAddress: msg.sender,
            providerHardwareId: hardwareId,
            accepted: false,
            clientDeposit: msg.value,
            providerDeposit: 0
        });

        // 8. emit event
        emit SlaProposal(
            msg.sender,
            hardwareId,
            contractId,
            client.dhKeyX,
            client.dhKeyY
        );

        return contractId;
    } // end proposeContract

    /**
     * Provider accepts a client's proposal
     * @param contractId The ID of the contract
     * @param providerMessage The message from the provider (includes hostname)
     */
    function acceptProposal(
        uint64 contractId,
        bytes memory providerMessage
    )
        external payable
        returns (address)
    {
        // 1. check that the proposal exists
        require(
            contractId < m_proposeCount,
            "Proposal not found"
        );

        ClientProposal storage proposal = m_clientProposalMap[contractId];
        Provider storage provider = m_providerMap[proposal.providerHardwareId];

        // check tx.origin matches provider address
        require(
            tx.origin == provider.walletAddr,
            "Provider address mismatch"
        );

        // 2. check that the proposal has not already been accepted
        require(
            proposal.accepted == false,
            "Proposal already accepted"
        );

        // 3. check that the provider has enough deposit
        require(
            msg.value >= m_minDepositWei,
            "Provider deposit too low"
        );

        // 4. update the provider's deposit
        proposal.providerDeposit = msg.value;

        // 5. update the mapping
        proposal.accepted = true;

        // 6. create a new SLA contract
        SLA slaInst = (new SLA){
            value: proposal.clientDeposit + proposal.providerDeposit
        }(
            m_reputationAddr,
            proposal.providerHardwareId,
            provider.appKeyAddress,
            provider.walletAddr,
            proposal.clientWalletAddress,
            proposal.providerDeposit,
            proposal.clientDeposit,
            provider.rate,
            contractId
        );

        // 7. get the address of the SLA contract
        address slaAddr = address(slaInst);

        // 8. update the mapping
        m_slaContractMap[contractId] = SLAContract({
            slaAddr: slaAddr,
            isActive: true
        });

        // 9. emit event
        emit SlaProposalAccepted(
            contractId,
            proposal.clientWalletAddress,
            proposal.providerHardwareId,
            slaAddr,
            providerMessage
        );

        return slaAddr;
    } // end acceptProposal

    /**
     * Add a provider to the revocation list
     * @param hardwareId The hardware ID of the provider enclave
     */
    function revokeProvider(bytes32 hardwareId) external {
        // make sure the provider has not already been revoked
        require(
            m_revokedProvidersMap[hardwareId] == false,
            "Provider already revoked"
        );

        // update the mapping
        m_revokedProvidersMap[hardwareId] = true;
    }

    /**
     * Finalize an SLA contract. Called by the Sla contract to deposit funds
       back into the client and provider accounts
     * @param contractId The ID of the SLA contract
     */
    function finalizeSlaContract(
        uint256 contractId
    )
        external payable
    {
        // deactivate the contract
        m_slaContractMap[contractId].isActive = false;

        // emit event
        emit SlaContractFinalized(contractId);
    }

    // get reputation address
    function getReputationAddress() external view returns (address) {
        return m_reputationAddr;
    }

    // get the proposal count
    function getProposalCount() external view returns (uint256) {
        return m_proposeCount;
    }

} // end contract SlaManager
