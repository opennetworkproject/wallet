#include <opennetworktests/test_utils.h>

void initState(){
    boost::filesystem::path pathTemp;		
    pathTemp = GetTempPath() / strprintf("test_bitcoin_%lu_%i", (unsigned long)GetTime(), (int)(GetRand(100000)));
    boost::filesystem::create_directories(pathTemp);
    const std::string dirOpennetwork = pathTemp.string();
    const dev::h256 hashDB(dev::sha3(dev::rlp("")));
    globalState = std::unique_ptr<OpennetworkState>(new OpennetworkState(dev::u256(0), OpennetworkState::openDB(dirOpennetwork, hashDB, dev::WithExisting::Trust), dirOpennetwork + "/opennetworkDB", dev::eth::BaseState::Empty));

    globalState->setRootUTXO(dev::sha3(dev::rlp(""))); // temp
}

CBlock generateBlock(){
    CBlock block;
    CMutableTransaction tx;
    std::vector<unsigned char> address(ParseHex("abababababababababababababababababababab"));
    tx.vout.push_back(CTxOut(0, CScript() << OP_DUP << OP_HASH160 << address << OP_EQUALVERIFY << OP_CHECKSIG));
    block.vtx.push_back(MakeTransactionRef(CTransaction(tx)));
    return block;
}

dev::Address createOpennetworkAddress(dev::h256 hashTx, uint32_t voutNumber){
    uint256 hashTXid(h256Touint(hashTx));
    std::vector<unsigned char> txIdAndVout(hashTXid.begin(), hashTXid.end());
    std::vector<unsigned char> voutNumberChrs;
    if (voutNumberChrs.size() < sizeof(voutNumber))voutNumberChrs.resize(sizeof(voutNumber));
    std::memcpy(voutNumberChrs.data(), &voutNumber, sizeof(voutNumber));
    txIdAndVout.insert(txIdAndVout.end(),voutNumberChrs.begin(),voutNumberChrs.end());

    std::vector<unsigned char> SHA256TxVout(32);
    CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());

    std::vector<unsigned char> hashTxIdAndVout(20);
    CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(hashTxIdAndVout.data());

    return dev::Address(hashTxIdAndVout);
}


OpennetworkTransaction createOpennetworkTransaction(valtype data, dev::u256 value, dev::u256 gasLimit, dev::u256 gasPrice, dev::h256 hashTransaction, dev::Address recipient, int32_t nvout){
    OpennetworkTransaction txEth;
    if(recipient == dev::Address()){
        txEth = OpennetworkTransaction(value, gasPrice, gasLimit, data, dev::u256(0));
    } else {
        txEth = OpennetworkTransaction(value, gasPrice, gasLimit, recipient, data, dev::u256(0));
    }
    txEth.forceSender(dev::Address("0101010101010101010101010101010101010101"));
    txEth.setHashWith(hashTransaction);
    txEth.setNVout(nvout);
    txEth.setVersion(VersionVM::GetEVMDefault());
    return txEth;
}

std::pair<std::vector<ResultExecute>, ByteCodeExecResult> executeBC(std::vector<OpennetworkTransaction> txs){
    CBlock block(generateBlock());
    OpennetworkDGP opennetworkDGP(globalState.get(), fGettingValuesDGP);
    uint64_t blockGasLimit = opennetworkDGP.getBlockGasLimit(chainActive.Tip()->nHeight + 1);
    ByteCodeExec exec(block, txs, blockGasLimit);
    exec.performByteCode();
    std::vector<ResultExecute> res = exec.getResult();
    ByteCodeExecResult bceExecRes;
    exec.processingResults(bceExecRes); //error handling?
    globalState->db().commit();
    globalState->dbUtxo().commit();
    return std::make_pair(res, bceExecRes);
}
