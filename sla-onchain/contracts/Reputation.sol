// SPDX-License-Identifier: MIT
pragma solidity ^0.8.17;

contract Reputation {

    //===== structs =====

    struct Provider {
        bool registered;
        uint256 numRequestsProcessed;
        uint256 reputation;
    }

    struct Client {
        bool registered;
        uint64 numDisputes;
        uint256 reputation;
    }

    //===== Member variables =====

    mapping(address => Client)   m_clientMap;
    mapping(bytes32 => Provider) m_providerMap;

    uint64 constant INITIAL_REP = 10000000;
    uint64 constant PENALTY_PER_DISPUTE = 100;

    //===== Events =====
    event ReputationContractDeployed(address indexed reputationAddr);
    event ReputationEvent(address indexed reputationAddr, bytes32 providerHardwareId, address clientAddr, string details);

    //===== Constructor =====

    constructor() {
        emit ReputationContractDeployed(address(this));
    }

    //===== Modifiers =====
    // modifier onlyParticipants(address providerAddr, address clientAddr) {
    modifier onlyParticipants(bytes32 providerHardwareId, address clientAddr) {
        require(
            m_providerMap[providerHardwareId].registered,
            "Provider not registered"
        );

        require(
            m_clientMap[clientAddr].registered,
            "Client not registered"
        );

        _;
    }

    //===== external Functions =====

    /**
     * Register a provider to the reputation contract
     * @param providerHardwareId The hardware id of the provider's enclave
     */
    function registerProvider(bytes32 providerHardwareId) external {
        require(
            !m_providerMap[providerHardwareId].registered,
            "Provider already registered"
        );

        m_providerMap[providerHardwareId].registered = true;
        m_providerMap[providerHardwareId].reputation = INITIAL_REP;
    }

    /**
     * Register a client to the reputation contract
     * @param clientAddr The address of the client
     */
    function registerClient(address clientAddr) external {
        require(
            !m_clientMap[clientAddr].registered,
            "Client already registered"
        );

        m_clientMap[clientAddr].registered = true;
        m_clientMap[clientAddr].reputation = INITIAL_REP;
    }


    /**
     * Calculate the reputation for a checkpoint report
     * @param providerHardwareId the hardware id of the provider's enclave
     * @param clientAddr The address of the client
     * @param paymentAmount The amount of payment in GWei
     */
    function onCheckpointReport(
        bytes32 providerHardwareId,
        address clientAddr,
        uint256 paymentAmount
    )
        external onlyParticipants(providerHardwareId, clientAddr)
    {
        /** 1. calculate the reputation using the following formula
         *  repUpAmount = payAmountGWei * weight
         *  weight = (providerRep + clientRep) / (2 * INITIAL_REP) / 10
         *
         *  Note: weight is divided by 10 to prevent the reputation from
         *  increasing too quickly in case of collusion between the provider
         *  and client
         */
        uint256 weight = ((m_providerMap[providerHardwareId].reputation +
                         m_clientMap[clientAddr].reputation) /
                         INITIAL_REP) * 2;

        uint256 repUpAmount = paymentAmount * weight;

        // 2. add the reputation to the provider and client
        m_providerMap[providerHardwareId].reputation += repUpAmount;
        m_clientMap[clientAddr].reputation += repUpAmount;

        // 3. emit event
        emit ReputationEvent(address(this), providerHardwareId, clientAddr, "CHECKPOINT REPORT");
    }

    /**
     * Calculate the reputation for a resolved dispute
     * @param providerHardwareId The address of the provider
     * @param clientAddr The address of the client
     */
    function onResolvedDispute(
        bytes32 providerHardwareId,
        address clientAddr
    )
        external onlyParticipants(providerHardwareId, clientAddr)
    {
        uint256 repDownAmount = m_clientMap[clientAddr].reputation / INITIAL_REP;

        // 1. subtract the reputation from the provider and client
        // check overflow
        if (m_providerMap[providerHardwareId].reputation < repDownAmount) {
            m_providerMap[providerHardwareId].reputation = 0;
        } else {
            m_providerMap[providerHardwareId].reputation -= repDownAmount;
        }

        if (m_clientMap[clientAddr].reputation < repDownAmount) {
            m_clientMap[clientAddr].reputation = 0;
        } else {
            m_clientMap[clientAddr].reputation -= repDownAmount;
        }

        // 2. emit event
        emit ReputationEvent(address(this), providerHardwareId, clientAddr, "RESOLVED DISPUTE");
    }

    /**
     * Calculate the reputation for any unresolved disputes
     * @param providerHardwareId The address of the provider
     * @param clientAddr The address of the client
     * @param numUnresolvedDisputes The number of unresolved disputes
     */
    function onUnresolvedDispute(
        bytes32 providerHardwareId,
        address clientAddr,
        uint64  numUnresolvedDisputes
    )
        external onlyParticipants(providerHardwareId, clientAddr)
    {
        // 1. calculate the reputation
        uint256 weight = (m_clientMap[clientAddr].reputation / INITIAL_REP) * 1000;

        uint256 providerDownAmount = PENALTY_PER_DISPUTE * numUnresolvedDisputes * weight;
        uint256 clientUpAmount = weight;

        // 2. subtract the reputation from the provider
        // check overflow
        if (m_providerMap[providerHardwareId].reputation < providerDownAmount) {
            m_providerMap[providerHardwareId].reputation = 0;
        } else {
            m_providerMap[providerHardwareId].reputation -= providerDownAmount;
        }

        // 3. add the reputation to the client
        m_clientMap[clientAddr].reputation += clientUpAmount;

        // 4. construct event details
        string memory details = string(abi.encodePacked(
            "UNRESOLVED DISPUTE: ",numUnresolvedDisputes));

        // 5. emit event
        emit ReputationEvent(address(this), providerHardwareId, clientAddr, details);
    }

    // ===== View Functions =====

    // get the host reputation
    function getProviderReputation(bytes32 providerHardwareId) external view returns (uint256) {
        return m_providerMap[providerHardwareId].reputation;
    }

    // get the client reputation
    function getClientReputation(address clientAddr) external view returns (uint256) {
        return m_clientMap[clientAddr].reputation;
    }

} // end of contract Reputation