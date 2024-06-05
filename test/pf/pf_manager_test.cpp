#include "gtest/gtest.h"
#include "pf/pf_manager.h"

namespace redbase {

TEST(PFManagerTest, TestGetPage) {
    std::string db_fname = "test.db";
    remove("test.db");


    auto pf_manager = std::make_shared<PF_Manager>(db_fname);

    char *dt = new char[1 << 12];

    std::string test1 = "x 123123";

    memset(dt, 0, 1 << 12);
    memcpy(dt, test1.c_str(), test1.size());

    pf_manager->WritePage(0, dt);

    memset(dt, 0, 1 << 12);
    pf_manager->ReadPage(0, dt);
    EXPECT_EQ(test1, std::string(dt));

    std::cout << dt << std::endl;

    // write page 1
    std::string test2 = "cxz";
    memset(dt, 0, 1 << 12);
    memcpy(dt, test2.c_str(), test2.size());

    pf_manager->WritePage(1, dt);
    
    memset(dt, 0, 1 << 12);
    pf_manager->ReadPage(1, dt);

    EXPECT_EQ(test2, std::string(dt));
    std::cout << dt << std::endl;

    pf_manager->Shutdown();
    delete[] dt;
}


TEST(PFManagerTest, TestFile) {
//  remove("xxx.t");
  std::fstream a("xxx.t", std::ios::app);

  a.seekp(0);
  a.write("123", 3);

  std::cout << a.tellp() << a.tellg() << std::endl;
  a.seekp(0, std::ios::end);
  std::cout << a.tellp() << a.tellg() << std::endl;

  a.close();

}


} // namespace redbase
