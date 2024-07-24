/*
 * MIT License
 *
 * Copyright (c) 2020 International Business Machines
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "helayers/hebase/hebase.h"
#include "helayers/hebase/palisade/PalisadeCkksContext.h"
#include "helayers/hebase/mockup/MockupContext.h"
#include "helayers/hebase/InitProtocol.h"
#include "helayers/hebase/DecryptProtocol.h"
#include "helayers/hebase/MultiPartyUtils.h"
#include "helayers/hebase/ProtocolMessage.h"
#include "helayers/hebase/HelayersTimer.h"
#include "helayers/hebase/utils/HelayersConfig.h"
#include "helayers/hebase/utils/TextIoUtils.h"
#include "helayers/ai/logistic_regression/LogisticRegression.h"
#include "helayers/ai/logistic_regression/LogisticRegressionPlain.h"
#include "helayers/ai/HeModel.h"
#include "helayers/ai/AiGlobals.h"
#include "helayers/math/MathGlobals.h"

#include <vector>
#include <numeric>
#include <filesystem>
#include <thread>

using namespace std;
using namespace helayers;

/*
This example demonstrates a use of a multi-party FHE setting. In this setting, a
set of parties wish to compute some function over their secret data while not
revealing it to any of the other parties. Using a regular public-key setting
will not be secure, since it requires the parties to trust the holder of the
secret key (whether it is one of the parties or a "trusted" third party). In the
multi-party FHE setting, none of the parties has a hold on the secret key.
Instead, each party has its own secret key (therefore, it will also be called a
"key-owner" later on). The public keys (which includes the encryption key and
the evaluation keys) are generated in a initialization protocol (a.k.a
InitProtocol) between the parties. To decrypt a ciphertext, each of the parties
(key-owners) needs to give its consent and to take part in a decryption protocol
(a.k.a. DecryptProtocol).

In the following example we consider the case of 2 data owners - Alice and Bob -
and a server. The security model requires that the server is not colluding with
any of the data owners. The example demonstrates how an encrypted linear
regression model can be trained with encrypted data of multiples data owners.
After the training process is complete the model is decrypted and shared between
the data owners. The model used in this example is a linear regression model.

The input of each of the data owners (Alice and Bob) is a dataset of 100000
samples and 1 feature contains fabricated data.
*/

const std::string outDir = getExamplesOutputDir();
double meanX = 10.0;
double stdX = 1.0;
double stdXNoiseForFabricatedData = 0.1;

double phi0 = 50.0;
double phi1 = -0.4;

int seed = 17;
int numSamplesEachParty = 100000;

bool useMockup = false;

shared_ptr<HeContext> getUninitializedContext();
void setupParticipant(shared_ptr<HeContext> he,
                      const string& name,
                      HeConfigRequirement req);
shared_ptr<HeModel> setupHeModel(shared_ptr<HeContext> he,
                                 const PlainModelHyperParams& hyperParams);
void generateEncryptAndSaveInputs(shared_ptr<HeContext> he,
                                  const string& name,
                                  shared_ptr<HeModel> lr);
void readMessagesAndExecuteRound(shared_ptr<HeContext> he, Protocol& protocol);

int main()
{
  cout << "*** Starting multi-party FHE demo ***" << endl;

  // These are HE requirements shared by the participants.
  HeConfigRequirement req =
      useMockup ? HeConfigRequirement::insecure(pow(2, 13), 9, 42, 10)
                : HeConfigRequirement(pow(2, 13), 9, 42, 10);
  req.publicFunctions.rotate = CUSTOM_ROTATIONS;
  req.publicFunctions.rotationSteps = {1, 16, 256, 4096};

  // === Alice setup ===
  shared_ptr<HeContext> heAlice = getUninitializedContext();
  setupParticipant(heAlice, "alice", req);

  // === Bob setup ===
  shared_ptr<HeContext> heBob = getUninitializedContext();
  setupParticipant(heBob, "bob", req);

  // === Server setup ===
  shared_ptr<HeContext> heServer = getUninitializedContext();
  setupParticipant(heServer, "server", req);

  // ====== Initialization protocol ======
  cout << "*** Initialization protocol ***" << endl;
  FileUtils::createCleanDir(outDir);

  // === Alice side ===
  InitProtocol initProtocolAlice(*heAlice);

  // === Bob side ===
  InitProtocol initProtocolBob(*heBob);

  // === Server side ===
  InitProtocol initProtocolServer(*heServer);

  while (initProtocolAlice.needsAnotherRound()) {
    // === Alice side ===
    thread thAlice(
        readMessagesAndExecuteRound, heAlice, ref(initProtocolAlice));

    // === Bob side ===
    thread thBob(readMessagesAndExecuteRound, heBob, ref(initProtocolBob));

    // === Server side ===
    thread thServer(
        readMessagesAndExecuteRound, heServer, ref(initProtocolServer));

    thAlice.join();
    thBob.join();
    thServer.join();
  }
  // ====== End of Initialization Protocol ======

  // At this point all parties have the public keys, and they can encrypt and
  // perform homomorphic computation. Alice and Bob will each initialize an
  // empty HE model and use it to encrypt their inputs and send them to the
  // server.

  cout << "*** Encrypt inputs ***" << endl;

  FileUtils::createCleanDir(outDir);

  // These are hyper parameters shared by the parties
  PlainModelHyperParams hyperParams;
  hyperParams.numberOfFeatures = 1;
  hyperParams.logisticRegressionActivation = LR_ACTIVATION_NONE;
  hyperParams.linearRegressionDistributionX = LR_NORMAL_DISTRIBUTION;
  hyperParams.linearRegressionMeanX = meanX;
  hyperParams.linearRegressionStdX = stdX;
  hyperParams.inverseApproximationPrecision = 2;
  hyperParams.trainable = true;
  hyperParams.verbose = false;

  // This should equal the number of data owners. In our case there are two
  // data owners (Alice and Bob).
  hyperParams.fitHyperParams.numberOfIterations = 2;

  // This should equal the maximal number of samples for each party. In our case
  // there are at most numSamplesEachParty samples for each party.
  hyperParams.fitHyperParams.fitBatchSize = numSamplesEachParty;

  // === Alice side ===
  shared_ptr<HeModel> lrAlice = setupHeModel(heAlice, hyperParams);
  generateEncryptAndSaveInputs(heAlice, "alice", lrAlice);

  // === Bob side ===
  shared_ptr<HeModel> lrBob = setupHeModel(heBob, hyperParams);
  generateEncryptAndSaveInputs(heBob, "bob", lrBob);

  // The server initializes an empty HE model, loads Alice's and Bob's encrypted
  // inputs and trains the model.

  cout << "*** Train encrypted model ***" << endl;

  // === Server side ===
  EncryptedData ed0(*heServer), ed1(*heServer);
  ed0.loadFromFile(outDir + "/alice_encrypted_data");
  ed1.loadFromFile(outDir + "/bob_encrypted_data");

  // This merges the two EncryptedData elements into one element.
  ed0.addEncryptedData(ed1);

  shared_ptr<HeModel> lrServer = setupHeModel(heServer, hyperParams);
  lrServer->fit(ed0);

  // The server extracts the encrypted internals of the trained model.
  EncryptedData encryptedModelInternals = lrServer->getEncryptedInternals();

  // Now, Alice and Bob wish to decrypt the model. Alice will be the
  // plaintext-aggregator (i.e., the one who gets the decrypted model first),
  // and she will share the result with Bob.

  // ====== Decryption Protocol ======
  cout << "*** Decryption protocol ***" << endl;

  // Clear the directory
  FileUtils::createCleanDir(outDir);

  // The IDs are known by each of the participants
  int32_t aliceId =
      heAlice->getHeConfigRequirement().multiPartyConfig->participantId;

  // === Alice side ===
  DecryptProtocol decryptProtocolAlice(*heAlice);
  decryptProtocolAlice.setPlaintextAggregatorId(aliceId);

  // === Bob side ===
  DecryptProtocol decryptProtocolBob(*heBob);
  decryptProtocolBob.setPlaintextAggregatorId(aliceId);

  // === Server side ===
  DecryptProtocol decryptProtocolServer(*heServer);
  decryptProtocolServer.setPlaintextAggregatorId(aliceId);

  // The server also needs to load the ciphertext
  decryptProtocolServer.setInput(encryptedModelInternals);

  while (decryptProtocolAlice.needsAnotherRound()) {
    thread thAlice(
        readMessagesAndExecuteRound, heAlice, ref(decryptProtocolAlice));

    // === Bob side ===
    thread thBob(readMessagesAndExecuteRound, heBob, ref(decryptProtocolBob));

    // === Server side ===
    thread thServer(
        readMessagesAndExecuteRound, heServer, ref(decryptProtocolServer));

    thAlice.join();
    thBob.join();
    thServer.join();
  }

  // Alice gets the output
  vector<DoubleTensorCPtr> decodedModelInternals =
      decryptProtocolAlice.getOutputVectorDoubleTensorCPtr();
  // ====== End of Decrypt Protocol ======

  // Alice uses the decoded model internals to build a plain model.
  shared_ptr<PlainModel> trainedPlainModel =
      lrAlice->getPlainModelFromDecodedInternals(decodedModelInternals);

  // Check result
  double trainedPhi0 =
      dynamic_pointer_cast<LogisticRegressionPlain>(trainedPlainModel)
          ->getWeights()
          .at(0);
  double trainedPhi1 =
      dynamic_pointer_cast<LogisticRegressionPlain>(trainedPlainModel)
          ->getBias()
          .at(0);
  double diffPhi0 = abs(phi0 - trainedPhi0);
  double diffPhi1 = abs(phi1 - trainedPhi1);
  always_assert_msg(
      diffPhi0 < 1e-1 && diffPhi1 < 1e-1,
      "trained weights are too far from expected. Trained weight: (" +
          to_string(trainedPhi0) + ", " + to_string(trainedPhi1) +
          "); Expected: (" + to_string(phi0) + ", " + to_string(phi1) + ")");

  cout << "*** Checking results... OK ***" << endl;

  FileUtils::removeDir(outDir);

  return 0;
}

shared_ptr<HeContext> getUninitializedContext()
{
  return useMockup ? shared_ptr<HeContext>(make_shared<MockupContext>())
                   : shared_ptr<HeContext>(make_shared<PalisadeCkksContext>());
}

void setupParticipant(shared_ptr<HeContext> he,
                      const string& name,
                      HeConfigRequirement req)
{
  // Read multi-party configuration file
  MultiPartyConfig mpConfig;
  JsonWrapper jw;
  ifstream ifs = FileUtils::openIfstream(
      getDataSetsDir() + "/multi_party_fhe/multi_party_config_" + name +
      ".json");
  jw.load(ifs);
  mpConfig.fromJson(jw);
  req.multiPartyConfig = mpConfig;

  // Initialize context
  he->init(req);
}

shared_ptr<HeModel> setupHeModel(shared_ptr<HeContext> he,
                                 const PlainModelHyperParams& hyperParams)
{
  shared_ptr<PlainModel> lrp = PlainModel::create(hyperParams);
  HeRunRequirements heRunReq;

  if (useMockup) {
    heRunReq.setHeContextOptions({make_shared<PalisadeCkksContext>()});
  } else {
    heRunReq.setHeContextOptions({he});
  }
  heRunReq.setExplicitHeConfigRequirement(he->getHeConfigRequirement());

  optional<HeProfile> profile = HeModel::compile(*lrp, heRunReq);
  always_assert(profile.has_value());
  shared_ptr<HeModel> lr = lrp->getEmptyHeModel(*he);
  lr->encodeEncrypt(*lrp, *profile);
  return lr;
}

void generateEncryptAndSaveInputs(shared_ptr<HeContext> he,
                                  const string& name,
                                  shared_ptr<HeModel> lr)
{
  // get IO encoder from the HE model
  ModelIoEncoder modelIoEncoder(*lr);
  EncryptedData encryptedInputs(*he);

  // Create fabricated data
  DoubleTensor data =
      LinearRegressionEstimator::getSimulatedInputsNormalSimpleLR(
          phi0,
          phi1,
          meanX,
          stdX + stdXNoiseForFabricatedData,
          numSamplesEachParty,
          seed++);
  DoubleTensorCPtr x = make_shared<const DoubleTensor>(data.getSlice(1, 0));
  DoubleTensorCPtr y = make_shared<const DoubleTensor>(data.getSlice(1, 1));

  modelIoEncoder.encodeEncrypt(encryptedInputs, {x, y});
  encryptedInputs.saveToFile(outDir + "/" + name + "_encrypted_data");
}

void readMessagesAndExecuteRound(shared_ptr<HeContext> he, Protocol& protocol)
{
  vector<ProtocolMessage> inputMessages, outputMessages;

  // Read messages from directory (here we load every message and check its
  // metadata from the message object in memory. In other implementation we
  // can keep the metadata in the file name and save the loading of unneeded
  // messages).
  for (const auto& entry : filesystem::directory_iterator(outDir)) {
    string filename = entry.path().filename().generic_string();

    // Skip irrelevant messages
    const MultiPartyConfig& mpConfig =
        *he->getHeConfigRequirement().multiPartyConfig;
    if (filename.find("round_" + to_string(protocol.getCurrentRound()) + "_") ==
            string::npos ||
        filename.find("source_id_" + to_string(mpConfig.participantId) + "_") !=
            string::npos ||
        (filename.find("dest_role_AGGREGATOR") != string::npos &&
         !mpConfig.isAggregator()) ||
        (filename.find("dest_role_KEY-OWNER") != string::npos &&
         !mpConfig.isKeyOwner())) {
      continue;
    }

    ProtocolMessage message(*he);
    message.loadFromFile(entry.path());
    if (protocol.isInputMessageValidForCurrentRound(message)) {
      inputMessages.push_back(message);
    }
  }

  // Execute round
  bool result = protocol.executeNextRound(outputMessages, inputMessages);
  always_assert(result == true);

  // Upload messages to directory
  int i = 0;
  for (ProtocolMessage& message : outputMessages) {
    message.saveToFile(outDir + "/" + message.getMetadataAsString(true) +
                       to_string(i++));
  }
}