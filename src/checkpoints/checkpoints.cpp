// Copyright (c) 2017-2026, The Incognito Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "string_tools.h"
#include "storages/portable_storage_template_helper.h" // epee json include
#include "serialization/keyvalue_serialization.h"
#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <functional>
#include <vector>

using namespace epee;

#undef INCOGNITO_DEFAULT_LOG_CATEGORY
#define INCOGNITO_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
  /**
   * @brief struct for loading a checkpoint from json
   */
  struct t_hashline
  {
    uint64_t height; //!< the height of the checkpoint
    std::string hash; //!< the hash for the checkpoint
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(height)
          KV_SERIALIZE(hash)
        END_KV_SERIALIZE_MAP()
  };

  /**
   * @brief struct for loading many checkpoints from json
   */
  struct t_hash_json {
    std::vector<t_hashline> hashlines; //!< the checkpoint lines from the file
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(hashlines)
        END_KV_SERIALIZE_MAP()
  };

  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str, const std::string& difficulty_str)
  {
    crypto::hash h = crypto::null_hash;
    bool r = epee::string_tools::hex_to_pod(hash_str, h);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse checkpoint hash string into binary representation!");

    // return false if adding at a height we already have AND the hash is different
    if (m_points.count(height))
    {
      CHECK_AND_ASSERT_MES(h == m_points[height], false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
    }
    m_points[height] = h;
    if (!difficulty_str.empty())
    {
      try
      {
        difficulty_type difficulty(difficulty_str);
        if (m_difficulty_points.count(height))
        {
          CHECK_AND_ASSERT_MES(difficulty == m_difficulty_points[height], false, "Difficulty checkpoint at given height already exists, and difficulty for new checkpoint was different!");
        }
        m_difficulty_points[height] = difficulty;
      }
      catch (...)
      {
        LOG_ERROR("Failed to parse difficulty checkpoint: " << difficulty_str);
        return false;
      }
    }
    return true;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
  {
    return !m_points.empty() && (height <= (--m_points.end())->first);
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h, bool& is_a_checkpoint) const
  {
    auto it = m_points.find(height);
    is_a_checkpoint = it != m_points.end();
    if(!is_a_checkpoint)
      return true;

    if(it->second == h)
    {
      MINFO("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h);
      return true;
    }else
    {
      MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
      return false;
    }
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h) const
  {
    bool ignored;
    return check_block(height, h, ignored);
  }
  //---------------------------------------------------------------------------
  //FIXME: is this the desired behavior?
  bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const
  {
    if (0 == block_height)
      return false;

    auto it = m_points.upper_bound(blockchain_height);
    // Is blockchain_height before the first checkpoint?
    if (it == m_points.begin())
      return true;

    --it;
    uint64_t checkpoint_height = it->first;
    return checkpoint_height < block_height;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_max_height() const
  {
    if (m_points.empty())
      return 0;
    return m_points.rbegin()->first;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_nearest_checkpoint_height(uint64_t block_height) const
  {
    if (m_points.empty())
      return 0;

    auto it = m_points.upper_bound(block_height);
    if (it == m_points.begin())
      return 0;

    --it;
    return it->first;
  }
  //---------------------------------------------------------------------------
  const std::map<uint64_t, crypto::hash>& checkpoints::get_points() const
  {
    return m_points;
  }
  //---------------------------------------------------------------------------
  const std::map<uint64_t, difficulty_type>& checkpoints::get_difficulty_points() const
  {
    return m_difficulty_points;
  }

  bool checkpoints::check_for_conflicts(const checkpoints& other) const
  {
    for (auto& pt : other.get_points())
    {
      if (m_points.count(pt.first))
      {
        CHECK_AND_ASSERT_MES(pt.second == m_points.at(pt.first), false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
      }
    }
    return true;
  }

  bool checkpoints::init_default_checkpoints(network_type nettype)
  {
    if (nettype == TESTNET)
    {
      // Incognito testnet checkpoints - placeholder, no active testnet chain yet
      return true;
    }
    if (nettype == STAGENET)
    {
      // Incognito stagenet checkpoints - placeholder, no active stagenet chain yet
      return true;
    }
    // Incognito mainnet checkpoints - generated from explorer at scan.incognitolayer.pro
    // Includes hardfork heights: v1@0, v7@1, v8@69900, v9@70707, v10@106000, v11@581625
    ADD_CHECKPOINT(0, "1eabfd62a1de057baa1c5b3f6b1215ccd78e892256b1ed708b769a9e5ecc12f9");
    ADD_CHECKPOINT(1, "6c580e325fd6fde6efff5cdf00295ecb8d9c24e637ff184d719e591b6de06f71");
    ADD_CHECKPOINT(10, "084cfe4eb56d15556e06d3d31ed15ebf187be13d4104cd35696c2dc75257121e");
    ADD_CHECKPOINT(100, "faa6aa7e8528434a7c6a09a9c722e90573b9c110154d170a4d73d38424fcb83a");
    ADD_CHECKPOINT(1000, "b09493a44cd90a9ad021f382c5aa57a887976f713bf190be7d8c49235c9de03d");
    ADD_CHECKPOINT(9518, "135e5eace880bb8e1f5f57a429a17caa1d3a5c4250778d2cd698b5d3bfddc42d");
    ADD_CHECKPOINT(10000, "d8ffe10b99324a6ae38da321bc1f7622f8f5c9e56f3d33a852e69ea47856c353");
    ADD_CHECKPOINT(19037, "45793f98bc38b65e9b9f52e92f1059b93c5e3756f2c86e27b85e405cad80dac5");
    ADD_CHECKPOINT(28555, "1fb079d853f4a1878c51c5c8ab68d268e01b74c41872ac609823b91dce56152e");
    ADD_CHECKPOINT(38074, "e770e997f4db24df6e3f9baf5b17206520fcb3bf2a4fe64cbe3b5e318ceb29c9");
    ADD_CHECKPOINT(47593, "a135456d2dbe4274df45b4a27282faceb8f29182cc4f8921897663c4199253b6");
    ADD_CHECKPOINT(50000, "f9b63fea1565c40672bcff8a8b44c66ab17b8b5f1b0ebd24426a9ed078b00db6");
    ADD_CHECKPOINT(57111, "98487ffba6c7a832fc0861fa0bb8927aeed80b846e54729f93919e7bd02922ee");
    ADD_CHECKPOINT(66630, "86a0e1e13cd87a6efba234e75e2d4c310a263095f7cdc83bd2e548e2384c61a7");
    ADD_CHECKPOINT(69900, "b591f2f4fc6c165b9ced48fdc12d5146190a0664c34cd3353635c9e47c60507a");
    ADD_CHECKPOINT(70707, "77e0945e865395577ff5a4b06909d0b6ab185cdba6c8517eb9469084c6419aa0");
    ADD_CHECKPOINT(76149, "ecddd4581c3719fc8659bb47ed0e82eb7af0186a9769664ac0ee49dff5f67bbf");
    ADD_CHECKPOINT(85667, "710477e79ea80e7b6f16e3d3abce4a5a6a03042526a9eb3abf587f29f8b35cf1");
    ADD_CHECKPOINT(95186, "1143732fdb04024084da2c2ca108c6a65ecf89d2eb5a997628bc5733fb5c8f00");
    ADD_CHECKPOINT(100000, "d08bb7bcbca14d42586d2e635cc6c098ab04645b3c32a5c32483e24281418723");
    ADD_CHECKPOINT(104705, "6660edf399cb5c30aa7fca453fe0cb8ea517073bdf1b059234b08d77825d5c6e");
    ADD_CHECKPOINT(106000, "27d73326902f924a39797c784738c63b87b48b5c97fe8e252167ebf964e8cd51");
    ADD_CHECKPOINT(114223, "3ca301cf127abb456533735c254a71b522311d2e120e0fb95bc327d85a45c84d");
    ADD_CHECKPOINT(123742, "621d9558134d80afa850d059e8b0ab40d568020adb794dc82bef9c8531952fc0");
    ADD_CHECKPOINT(133261, "8966e1e812be2287450d6840f3c4645ee4020367a58fde43decd72e358718ca9");
    ADD_CHECKPOINT(142779, "31ba284d426b32360a3c1ed129ae04613cad95a81773271cfebb0b5e1ca39d6d");
    ADD_CHECKPOINT(150000, "febfacd055d64f5327734ab958422d46969d66e8d0ac303d40b72de23d0d1381");
    ADD_CHECKPOINT(152298, "36d3708a9e731a6f0dfd1b80d75e86ab0725ae4cd2c512d17e56a830f7de907a");
    ADD_CHECKPOINT(161817, "30c1f3e193b26de9a694374cdfaa2f37aa7d4869e992425db2f0a1abd7d892ae");
    ADD_CHECKPOINT(171335, "934947c50a8fdb0cd165de1703a21932a5a83284b3c9c4d326f1aeead26216d1");
    ADD_CHECKPOINT(180854, "dedf8cd23f3558841fb7b26379848a80c74c2ae31b5830b1dc2c9f15be22a9b8");
    ADD_CHECKPOINT(190372, "625af3bc367f5258784b64a2348973723b7a6d45a2bd36847e892d04df358d74");
    ADD_CHECKPOINT(199891, "de60b4111c8426f901eb9729efcaf3e0ab37476526f45080e93fea7cf331b048");
    ADD_CHECKPOINT(200000, "87d85c340048737b0304d4729f562ddcd7c1fc0cdd0c40e6577da04e74ff0749");
    ADD_CHECKPOINT(209410, "131f9d54626a979fad6e083f1dcb1a61fbe80040cb5685db2ced2a188ed4a66e");
    ADD_CHECKPOINT(218928, "acd3f5b1375fc2e959c22819f04c8a73f3d9b01e782f0bbef10b07ef4837c904");
    ADD_CHECKPOINT(228447, "22902313295dd8e9ea3c408c7d33adee604e5494993f142b2ed598018ab9da73");
    ADD_CHECKPOINT(237966, "b534a38940dbd75a00078aa9f5ffa611ba2f3bab82692ed830759800e0f1d9fc");
    ADD_CHECKPOINT(247484, "a46dd69c1f5fbf35994f2ed8cbe3ad66983408eec1a352abd036357a09120d6c");
    ADD_CHECKPOINT(250000, "e917d672dfcc6d2a2a98f8e7318517f0c291cef660e9ae629a0c7df66e72908c");
    ADD_CHECKPOINT(257003, "792b2699e0375c8bbdd63fc9d6fc7d24a5c346c70e8aa3d3be5101ad0212769c");
    ADD_CHECKPOINT(266522, "132557f0829dc465c89ed6d8e8939790205230648cac13d6afb1af8ac599d83d");
    ADD_CHECKPOINT(276040, "1472ef0917b6b508002612094eecbfaeeb12c1dabf3e1b43312b1fdf6423b3d9");
    ADD_CHECKPOINT(285559, "1f91e96495e4141ff83bbb5063ab236dfd5f99aef228b5b0d56d12fdd7a2068c");
    ADD_CHECKPOINT(295078, "813771c4c33522b945108e374ab6bc14edcdbc5ccc93e3f2569daa1a4448dd3c");
    ADD_CHECKPOINT(300000, "08a296defb2752a7d9e863472cab1689b3cd720680a8e080fc29b61b9d69e2b2");
    ADD_CHECKPOINT(304596, "69749094854503bd923fdeb40ede0c84760f9b08ef534ed3096363932a0e769b");
    ADD_CHECKPOINT(314115, "2580bcfa7e6a24917a1a8122459ae3961861baf75e2473ed55a9b88ac35a076e");
    ADD_CHECKPOINT(323634, "04fde4c4bf869a79785d6042fe7ff56df1a4dd68b9c0ba71a0b46980024a96f4");
    ADD_CHECKPOINT(333152, "d1dd11f7d43943734e5ca985b9d9b8d5ed5bc73a4bbf07bdf6d08be44e52dd90");
    ADD_CHECKPOINT(342671, "fbc150634b61b4d34787a8ba63b1897e7c4c806057cb0f9ab876b2294ac8ccc6");
    ADD_CHECKPOINT(350000, "639c2f81c9496b7eb631028f310fe71b0fa541f2854024be6016fc80b81640b3");
    ADD_CHECKPOINT(352190, "ded2032ef3ef3cc40c56dd4f81ea209ae027739493acdcf359dd6d28f7d4935f");
    ADD_CHECKPOINT(361708, "d4fbe3aab490b0b6c6711d5c27c81650ffb41788d6d24ceb6037bc0831a476e4");
    ADD_CHECKPOINT(371227, "a0abaaf541483b8645a919208c0d24f9f0b33903da7724f40a849b2888ea54d6");
    ADD_CHECKPOINT(380745, "371717c8bd5b4041016e557c47acfb8b40128d35ca15cb691d28be500ab5bd19");
    ADD_CHECKPOINT(390264, "5798b7115fbfcd354dd4cc5a03c100f376c598c6d38ffab90cf34244ae1a7aa1");
    ADD_CHECKPOINT(399783, "8c54ab73ffb98548eafe70d13c5b6b2486b40ddd20575ac490b22da9aff4799a");
    ADD_CHECKPOINT(400000, "be9027ef170dab086c90b628ddef8debd726204d25b9a7a6f7fdda4541c510b8");
    ADD_CHECKPOINT(409301, "46b0689b96569db1da94b58fb7077df15cd962df6ca65fc90d8152ad89f64476");
    ADD_CHECKPOINT(418820, "5769dd579cea9b176e5b8c34bfb52cfc56dd3c7f72c2ea5ca1bd6bd2c44f9547");
    ADD_CHECKPOINT(428339, "fedd1689ce6f71f718400a84ec73b258a6b0b47f9c232af3a8e42cce08236de3");
    ADD_CHECKPOINT(437857, "62d671d6fb64768df1dafa983a0c48c6ddef2a7c88a256227d7a127639c3aa01");
    ADD_CHECKPOINT(447376, "55cafc69e56dbe15c07fedcf639dc72e036cd942c074c6abcef401414e70fa3e");
    ADD_CHECKPOINT(450000, "f83c7c2ef9a4a1cb1c1a579054f890d23965594330d77120c1bd6ce90969b36c");
    ADD_CHECKPOINT(456895, "453ecefbbed0dcc08af0a57332990e863acd77134cc3b3e9f20c269b83b450ce");
    ADD_CHECKPOINT(466413, "71038b1e8632308fb8a8ddc249a81c76b90b2b8a7c948b2c2912d38483f691ed");
    ADD_CHECKPOINT(475932, "93e2a7cb06febaeb30abaeae9af28c0daff88899aab309ff92f25ae3393e2903");
    ADD_CHECKPOINT(485451, "b131ac3a475edff6fc84d4f6c1d89ebcab1d9275b1d14111a1455b7e68011687");
    ADD_CHECKPOINT(494969, "6a9df7481753e751f550ddca5ddd21e533b74da9b1c2047b0dd061253ccbaeb2");
    ADD_CHECKPOINT(500000, "3e6fb5b5a2c6760ceff433d47cf06f90d2632a4056acec65ecde59c044ad5f44");
    ADD_CHECKPOINT(504488, "7cf77143dc9ad5661129e5abdbe6474db5901837fa3acbbc752f6d72382a1c7a");
    ADD_CHECKPOINT(514007, "3db1fe68ef09dcb28181509436f8a7f9e22e0e9dd7265057acc9a65230aa84be");
    ADD_CHECKPOINT(523525, "74a23ab3380c2fb8caf0ca001b5416f891ada8cf75c0d32a7b61c3395f2311bf");
    ADD_CHECKPOINT(533044, "30e08f51e95e8a020a41ce687bc17bbd2052a42a62d67b592c2bd2adedb97c56");
    ADD_CHECKPOINT(542563, "892a634cd47ef68628dcc97c99726620fc7f1c84e77201ebb7e031caa83c0b92");
    ADD_CHECKPOINT(550000, "66fb3ed41212053c93051c86e85627a9bb049d5b9eb429c90bfc6ecd9b9862d4");
    ADD_CHECKPOINT(552081, "381277e27f3c36e21c21030037980099f276a54156ad96db57e94cff7b80f495");
    ADD_CHECKPOINT(561600, "7fe9b041c071a3d866981e7cbb6e24cdbc352707576fc5a128a12f0264bd25c3");
    ADD_CHECKPOINT(571118, "1dc013ce6cb821e56809021384ebfa935a426979f9c13650e465f9e58c58fcd5");
    ADD_CHECKPOINT(580637, "04dcdaceba2b9ed815903cd4d4277c672151580a305ffa3958ff3efc9daf3098");
    ADD_CHECKPOINT(581625, "8819fb71f478f8593a535a1c0f6359dab63f85facf63a415ccc7903849ce219a");
    ADD_CHECKPOINT(590156, "a2cff7d1481677633484ca5da538a344351ae8bc9d487be6c391562a72a5e42b");
    ADD_CHECKPOINT(599674, "7a2fc812f0df59661e0fb81031a9007a44e708fedf21d2bb3dfeae3730060cc4");
    ADD_CHECKPOINT(600000, "85cad24171e434d9e8c8f2bd7cccb93738e61f337a70b33086d65dcdc28469d0");
    ADD_CHECKPOINT(609193, "5a8b3b53d99787fec71d7ac4d3ae52fe527553c7efc544beb65057acac211275");
    ADD_CHECKPOINT(618712, "55ff52b8805878a6e6fe274a7d799c0d9844ebc2f1231485abd988c9334df20e");
    ADD_CHECKPOINT(628230, "881500f816ea9e126815baa5b0841d7223689fbd7fbba42b6773d9e687e59166");
    ADD_CHECKPOINT(637749, "ce369de79b6ceb81a728d67a97fa5eafa528c3c30f7521644e6819c527936b3d");
    ADD_CHECKPOINT(647268, "0b116c3367d178cb27191fcf4880b0ef67d8b594a655bde675002333d485471b");
    ADD_CHECKPOINT(650000, "994664e9804e97781ef8902085dc302e7aa477134355be586ae3df7c44178e92");
    ADD_CHECKPOINT(656786, "91ac8672f282edc1584de28f20d0263f1571472223e7ab7eb1d99e6923320a31");
    ADD_CHECKPOINT(666305, "0c36de37a5d801a755efc66a6f9880d6568185f1b4373effce36ab6c770825dc");
    ADD_CHECKPOINT(675824, "02b9a1f030f523174c8b9e79b351b14af7a7a84ae2bded503523500afc7784e3");
    ADD_CHECKPOINT(685342, "9f706dbc576ce531be47ac820b1d834d13955eef6d8171d4dfefd5231745c3d6");
    ADD_CHECKPOINT(694861, "c97a7e26499ab98b298266a6c8eadc1cfba7068fe2d1491096b29036615dc5d2");
    ADD_CHECKPOINT(700000, "089df442273dd43b7ea00e34bfd293f85121289cd47a3e9e3aef315b60b6619b");
    ADD_CHECKPOINT(704380, "8e3f8616c69458384d937af51d70bf3f23fc978e0868c02414c3f02cb0ec25a2");
    ADD_CHECKPOINT(713898, "2fd3b68cb7bc5520c81621c7bd1de6d2f34752552b720ce066245b5561fc44f9");
    ADD_CHECKPOINT(723417, "acdce69da54ffa55519808ded6e0265b80707d1ad281945cac7aebe1dc735070");
    return true;
  }

  bool checkpoints::load_checkpoints_from_json(const std::string &json_hashfile_fullpath)
  {
    boost::system::error_code errcode;
    if (! (boost::filesystem::exists(json_hashfile_fullpath, errcode)))
    {
      LOG_PRINT_L1("Blockchain checkpoints file not found");
      return true;
    }

    LOG_PRINT_L1("Adding checkpoints from blockchain hashfile");

    uint64_t prev_max_height = get_max_height();
    LOG_PRINT_L1("Hard-coded max checkpoint height is " << prev_max_height);
    t_hash_json hashes;
    if (!epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath))
    {
      MERROR("Error loading checkpoints from " << json_hashfile_fullpath);
      return false;
    }
    for (std::vector<t_hashline>::const_iterator it = hashes.hashlines.begin(); it != hashes.hashlines.end(); )
    {
      uint64_t height;
      height = it->height;
      if (height <= prev_max_height) {
        LOG_PRINT_L1("ignoring checkpoint height " << height);
      } else {
        std::string blockhash = it->hash;
        LOG_PRINT_L1("Adding checkpoint height " << height << ", hash=" << blockhash);
        ADD_CHECKPOINT(height, blockhash);
      }
      ++it;
    }

    return true;
  }

  bool checkpoints::load_checkpoints_from_dns(network_type nettype)
  {
    std::vector<std::string> records;

    // Incognito L1 - No DNS checkpoint domains configured yet
    // TODO: Set up DNS checkpoint domains when the network grows
    static const std::vector<std::string> dns_urls = {
    };

    static const std::vector<std::string> testnet_dns_urls = {
    };

    static const std::vector<std::string> stagenet_dns_urls = {
    };

    if (!tools::dns_utils::load_txt_records_from_dns(records, nettype == TESTNET ? testnet_dns_urls : nettype == STAGENET ? stagenet_dns_urls : dns_urls))
      return true; // why true ?

    for (const auto& record : records)
    {
      auto pos = record.find(":");
      if (pos != std::string::npos)
      {
        uint64_t height;
        crypto::hash hash;

        // parse the first part as uint64_t,
        // if this fails move on to the next record
        std::stringstream ss(record.substr(0, pos));
        if (!(ss >> height))
        {
    continue;
        }

        // parse the second part as crypto::hash,
        // if this fails move on to the next record
        std::string hashStr = record.substr(pos + 1);
        if (!epee::string_tools::hex_to_pod(hashStr, hash))
        {
    continue;
        }

        ADD_CHECKPOINT(height, hashStr);
      }
    }
    return true;
  }

  bool checkpoints::load_new_checkpoints(const std::string &json_hashfile_fullpath, network_type nettype, bool dns)
  {
    bool result;

    result = load_checkpoints_from_json(json_hashfile_fullpath);
    if (dns)
    {
      result &= load_checkpoints_from_dns(nettype);
    }

    return result;
  }
}
