#include <base/constants.h>
#include <base/file_utils.h>
#include <base/process_impl.h>
#include <base/test_process.h>
#include <base/temporary_dir.h>
#include <client/clang.h>
#include <client/command.h>
#include <client/flag_set.h>
#include <net/network_service_impl.h>
#include <net/test_network_service.h>
#include <proto/remote.pb.h>

#include <third_party/gtest/public/gtest/gtest.h>
#include <third_party/libcxx/exported/include/regex>

namespace dist_clang {
namespace client {

namespace {

// NOTICE: if changing something in these strings, make sure to apply the same
//         changes to the tests below.

// It's a possible output of the command:
// `cd /tmp; clang++ -### -c /tmp/test.cc`
const String clang_cc_output =
    "clang version 3.4 (...) (...)\n"
    "Target: x86_64-unknown-linux-gnu\n"
    "Thread model: posix\n"
    " \"/usr/bin/clang\" \"-cc1\""
    " \"-triple\" \"x86_64-unknown-linux-gnu\""
    " \"-emit-obj\""
    " \"-mrelax-all\""
    " \"-disable-free\""
    " \"-main-file-name\" \"test.cc\""
    " \"-mrelocation-model\" \"static\""
    " \"-mdisable-fp-elim\""
    " \"-fmath-errno\""
    " \"-masm-verbose\""
    " \"-mconstructor-aliases\""
    " \"-munwind-tables\""
    " \"-fuse-init-array\""
    " \"-target-cpu\" \"x86-64\""
    " \"-target-linker-version\" \"2.23.2\""
    " \"-coverage-file\" \"/tmp/test.o\""
    " \"-resource-dir\" \"/usr/lib/clang/3.4\""
    " \"-internal-isystem\" \"/usr/include/c++/4.8.2\""
    " \"-internal-isystem\" \"/usr/local/include\""
    " \"-internal-isystem\" \"/usr/lib/clang/3.4/include\""
    " \"-internal-externc-isystem\" \"/include\""
    " \"-internal-externc-isystem\" \"/usr/include\""
    " \"-fdeprecated-macro\""
    " \"-fdebug-compilation-dir\" \"/tmp\""
    " \"-ferror-limit\" \"19\""
    " \"-fmessage-length\" \"213\""
    " \"-mstackrealign\""
    " \"-fobjc-runtime=gcc\""
    " \"-fcxx-exceptions\""
    " \"-fexceptions\""
    " \"-fdiagnostics-show-option\""
    " \"-fcolor-diagnostics\""
    " \"-vectorize-slp\""
    " \"-o\" \"test.o\""
    " \"-x\" \"c++\""
    " \"/tmp/test.cc\"\n";
}  // namespace

TEST(FlagSetTest, SimpleInput) {
  String version;
  const String expected_version = "clang version 3.4 (...) (...)";
  FlagSet::CommandList input;
  const List<String> expected_input = {
      "",                          "/usr/bin/clang",
      "-cc1",                      "-triple",
      "x86_64-unknown-linux-gnu",  "-emit-obj",
      "-mrelax-all",               "-disable-free",
      "-main-file-name",           "test.cc",
      "-mrelocation-model",        "static",
      "-mdisable-fp-elim",         "-fmath-errno",
      "-masm-verbose",             "-mconstructor-aliases",
      "-munwind-tables",           "-fuse-init-array",
      "-target-cpu",               "x86-64",
      "-target-linker-version",    "2.23.2",
      "-coverage-file",            "/tmp/test.o",
      "-resource-dir",             "/usr/lib/clang/3.4",
      "-internal-isystem",         "/usr/include/c++/4.8.2",
      "-internal-isystem",         "/usr/local/include",
      "-internal-isystem",         "/usr/lib/clang/3.4/include",
      "-internal-externc-isystem", "/include",
      "-internal-externc-isystem", "/usr/include",
      "-fdeprecated-macro",        "-fdebug-compilation-dir",
      "/tmp",                      "-ferror-limit",
      "19",                        "-fmessage-length",
      "213",                       "-mstackrealign",
      "-fobjc-runtime=gcc",        "-fcxx-exceptions",
      "-fexceptions",              "-fdiagnostics-show-option",
      "-fcolor-diagnostics",       "-vectorize-slp",
      "-o",                        "test.o",
      "-x",                        "c++",
      "/tmp/test.cc"};
  auto it = expected_input.begin();

  EXPECT_TRUE(FlagSet::ParseClangOutput(clang_cc_output, &version, input));
  EXPECT_EQ(expected_version, version);
  for (const auto& value : *input.begin()) {
    ASSERT_EQ(*(it++), value);
  }

  proto::Flags expected_flags;
  expected_flags.mutable_compiler()->set_path("/usr/bin/clang");
  expected_flags.mutable_compiler()->set_version(expected_version);
  expected_flags.set_output("test.o");
  expected_flags.set_input("/tmp/test.cc");
  expected_flags.set_language("c++");
  expected_flags.add_other()->assign("-cc1");
  expected_flags.add_other()->assign("-triple");
  expected_flags.add_other()->assign("x86_64-unknown-linux-gnu");
  expected_flags.add_other()->assign("-disable-free");
  expected_flags.add_other()->assign("-mrelocation-model");
  expected_flags.add_other()->assign("static");
  expected_flags.add_other()->assign("-mdisable-fp-elim");
  expected_flags.add_other()->assign("-fmath-errno");
  expected_flags.add_other()->assign("-masm-verbose");
  expected_flags.add_other()->assign("-mconstructor-aliases");
  expected_flags.add_other()->assign("-munwind-tables");
  expected_flags.add_other()->assign("-fuse-init-array");
  expected_flags.add_other()->assign("-target-cpu");
  expected_flags.add_other()->assign("x86-64");
  expected_flags.add_other()->assign("-target-linker-version");
  expected_flags.add_other()->assign("2.23.2");
  expected_flags.add_other()->assign("-fdeprecated-macro");
  expected_flags.add_other()->assign("-fmessage-length");
  expected_flags.add_other()->assign("213");
  expected_flags.add_other()->assign("-mstackrealign");
  expected_flags.add_other()->assign("-fobjc-runtime=gcc");
  expected_flags.add_other()->assign("-fcxx-exceptions");
  expected_flags.add_other()->assign("-fexceptions");
  expected_flags.add_other()->assign("-fdiagnostics-show-option");
  expected_flags.add_other()->assign("-fcolor-diagnostics");
  expected_flags.add_other()->assign("-vectorize-slp");
  expected_flags.add_non_cached()->assign("-main-file-name");
  expected_flags.add_non_cached()->assign("test.cc");
  expected_flags.add_non_cached()->assign("-coverage-file");
  expected_flags.add_non_cached()->assign("/tmp/test.o");
  expected_flags.add_non_cached()->assign("-resource-dir");
  expected_flags.add_non_cached()->assign("/usr/lib/clang/3.4");
  expected_flags.add_non_cached()->assign("-internal-isystem");
  expected_flags.add_non_cached()->assign("/usr/include/c++/4.8.2");
  expected_flags.add_non_cached()->assign("-internal-isystem");
  expected_flags.add_non_cached()->assign("/usr/local/include");
  expected_flags.add_non_cached()->assign("-internal-isystem");
  expected_flags.add_non_cached()->assign("/usr/lib/clang/3.4/include");
  expected_flags.add_non_cached()->assign("-internal-externc-isystem");
  expected_flags.add_non_cached()->assign("/include");
  expected_flags.add_non_cached()->assign("-internal-externc-isystem");
  expected_flags.add_non_cached()->assign("/usr/include");
  expected_flags.add_non_cached()->assign("-fdebug-compilation-dir");
  expected_flags.add_non_cached()->assign("/tmp");
  expected_flags.add_non_cached()->assign("-ferror-limit");
  expected_flags.add_non_cached()->assign("19");
  expected_flags.add_cc_only()->assign("-mrelax-all");
  expected_flags.set_action("-emit-obj");

  proto::Flags actual_flags;
  actual_flags.mutable_compiler()->set_version(version);
  ASSERT_EQ(FlagSet::COMPILE,
            FlagSet::ProcessFlags(*input.begin(), &actual_flags));
  ASSERT_EQ(expected_flags.SerializeAsString(),
            actual_flags.SerializeAsString());
}

TEST(FlagSetTest, MultipleCommands) {
  String version;
  const String expected_version = "clang version 3.4 (...) (...)";
  FlagSet::CommandList input;
  const String clang_multi_output =
      "clang version 3.4 (...) (...)\n"
      "Target: x86_64-unknown-linux-gnu\n"
      "Thread model: posix\n"
      " \"/usr/bin/clang\""
      " \"-emit-obj\""
      " \"test.cc\"\n"
      " \"/usr/bin/objcopy\""
      " \"something\""
      " \"some_file\"\n";
  const List<String> expected_input1 = {"",          "/usr/bin/clang",
                                        "-emit-obj", "test.cc", };
  const List<String> expected_input2 = {"",          "/usr/bin/objcopy",
                                        "something", "some_file", };

  EXPECT_TRUE(FlagSet::ParseClangOutput(clang_multi_output, &version, input));
  EXPECT_EQ(expected_version, version);

  ASSERT_EQ(2u, input.size());

  auto it1 = expected_input1.begin();
  for (const auto& value : input.front()) {
    EXPECT_EQ(*(it1++), value);
  }

  auto it2 = expected_input2.begin();
  for (const auto& value : input.back()) {
    EXPECT_EQ(*(it2++), value);
  }

  proto::Flags actual_flags;
  actual_flags.mutable_compiler()->set_version(version);
  ASSERT_EQ(FlagSet::COMPILE,
            FlagSet::ProcessFlags(input.front(), &actual_flags));
  ASSERT_EQ(FlagSet::UNKNOWN,
            FlagSet::ProcessFlags(input.back(), &actual_flags));
}

TEST(CommandTest, NonExistentInput) {
  const int argc = 3;
  const char* argv[] = {"clang++", "-c", "/tmp/some_random.cc", nullptr};

  List<Command> commands;
  ASSERT_FALSE(Command::GenerateFromArgs(argc, argv, commands));
  ASSERT_TRUE(commands.empty());
}

TEST(CommandTest, MissingArgument) {
  const int argc = 4;
  const char* argv[] = {"clang++", "-I", "-c", "/tmp/some_random.cc", nullptr};

  List<Command> commands;
  ASSERT_FALSE(Command::GenerateFromArgs(argc, argv, commands));
  ASSERT_TRUE(commands.empty());
}

TEST(CommandTest, UnknownArgument) {
  const int argc = 4;
  const char* argv[] = {"clang++", "-12", "-c", "/tmp/some_random.cc", nullptr};

  List<Command> commands;
  ASSERT_FALSE(Command::GenerateFromArgs(argc, argv, commands));
  ASSERT_TRUE(commands.empty());
}

TEST(CommandTest, ParseSimpleArgs) {
  const String temp_input = base::CreateTempFile(".cc");
  const String expected_output = "/tmp/output.o";
  std::list<std::regex> expected_regex;
  expected_regex.emplace_back("-cc1");
  expected_regex.emplace_back("-triple [a-z0-9_]+-[a-z0-9_]+-[a-z0-9]+");
  expected_regex.emplace_back("-emit-obj");
  expected_regex.emplace_back("-mrelax-all");
  expected_regex.emplace_back("-disable-free");
  expected_regex.emplace_back("-main-file-name clangd-[a-zA-Z0-9]+\\.cc");
  expected_regex.emplace_back("-mrelocation-model static");
  expected_regex.emplace_back("-mdisable-fp-elim");
  expected_regex.emplace_back("-fmath-errno");
  expected_regex.emplace_back("-masm-verbose");
  expected_regex.emplace_back("-mconstructor-aliases");
  expected_regex.emplace_back("-munwind-tables");
  expected_regex.emplace_back("-fuse-init-array");
  expected_regex.emplace_back("-target-cpu [a-z0-9_]+");
  expected_regex.emplace_back("-target-linker-version [0-9.]+");
  expected_regex.emplace_back("-coverage-file " + expected_output);
  expected_regex.emplace_back("-resource-dir");
  expected_regex.emplace_back("-internal-isystem");
  expected_regex.emplace_back("-internal-externc-isystem");
  expected_regex.emplace_back("-fdeprecated-macro");
  expected_regex.emplace_back("-fdebug-compilation-dir");
  expected_regex.emplace_back("-ferror-limit [0-9]+");
  expected_regex.emplace_back("-fmessage-length [0-9]+");
  expected_regex.emplace_back("-mstackrealign");
  expected_regex.emplace_back("-fobjc-runtime=");
  expected_regex.emplace_back("-fcxx-exceptions");
  expected_regex.emplace_back("-fexceptions");
  expected_regex.emplace_back("-fdiagnostics-show-option");
  expected_regex.emplace_back("-vectorize-slp");
  expected_regex.emplace_back("-o " + expected_output);
  expected_regex.emplace_back("-x c++");
  expected_regex.emplace_back(temp_input);

  const char* argv[] = {"clang++", "-c",                    temp_input.c_str(),
                        "-o",      expected_output.c_str(), nullptr};
  const int argc = 5;

  List<Command> commands;
  ASSERT_TRUE(Command::GenerateFromArgs(argc, argv, commands));
  ASSERT_EQ(1u, commands.size());

  const auto& command = commands.front();
  for (const auto& regex : expected_regex) {
    EXPECT_TRUE(std::regex_search(command.RenderAllArgs(), regex));
  }

  // TODO: check |FillArgs()|.
}

class ClientTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    using Service = net::TestNetworkService;

    {
      auto factory = net::NetworkService::SetFactory<Service::Factory>();
      factory->CallOnCreate([this](Service* service) {
        service->CountConnectAttempts(&connect_count);
        service->CallOnConnect([this](net::EndPointPtr, String* error) {
          if (!do_connect) {
            if (error) {
              error->assign("Test service rejects connection intentionally");
            }
            return Service::TestConnectionPtr();
          }

          auto connection = Service::TestConnectionPtr(new net::TestConnection);
          connection->CountSendAttempts(&send_count);
          connection->CountReadAttempts(&read_count);
          weak_ptr = connection->shared_from_this();
          ++connections_created;
          connect_callback(connection.get());

          return connection;
        });
      });
    }

    {
      auto factory = base::Process::SetFactory<base::TestProcess::Factory>();
      factory->CallOnCreate([this](base::TestProcess* process) {
        process->CallOnRun([this, process](ui32 timeout, const String& input,
                                           String* error) {
          // Client shouldn't run external processes.
          return false;
        });
      });
    }
  }

 protected:
  bool do_connect = true;
  net::ConnectionWeakPtr weak_ptr;
  ui32 send_count = 0, read_count = 0, connect_count = 0,
       connections_created = 0;
  Fn<void(net::TestConnection*)> connect_callback = EmptyLambda<>();
};

TEST_F(ClientTest, NoConnection) {
  const String temp_input = base::CreateTempFile(".cc");
  const char* argv[] = {"clang++", "-c", temp_input.c_str(), nullptr};
  const int argc = 3;

  do_connect = false;
  EXPECT_TRUE(client::DoMain(argc, argv, "socket_path", "clang_path"));
  EXPECT_TRUE(weak_ptr.expired());
  EXPECT_EQ(0u, send_count);
  EXPECT_EQ(0u, read_count);
  EXPECT_EQ(1u, connect_count);
  EXPECT_EQ(0u, connections_created);
}

TEST_F(ClientTest, NoEnvironmentVariable) {
  const String temp_input = base::CreateTempFile(".cc");
  const char* argv[] = {"clang++", "-c", temp_input.c_str(), nullptr};
  const int argc = 3;

  EXPECT_TRUE(client::DoMain(argc, argv, String(), String()));
  EXPECT_TRUE(weak_ptr.expired());
  EXPECT_EQ(0u, send_count);
  EXPECT_EQ(0u, read_count);
  EXPECT_EQ(0u, connect_count);
  EXPECT_EQ(0u, connections_created);
}

TEST_F(ClientTest, NoInputFile) {
  const char* argv[] = {"clang++", "-c", "/tmp/qwerty", nullptr};
  const int argc = 3;

  EXPECT_TRUE(client::DoMain(argc, argv, "socket_path", "clang++"));
  EXPECT_TRUE(weak_ptr.expired());
  EXPECT_EQ(0u, send_count);
  EXPECT_EQ(0u, read_count);
  EXPECT_EQ(0u, connect_count);
  EXPECT_EQ(0u, connections_created);
}

TEST_F(ClientTest, CannotSendMessage) {
  const String temp_input = base::CreateTempFile(".cc");
  const char* argv[] = {"clang++", "-c", temp_input.c_str(), nullptr};
  const int argc = 3;

  connect_callback = [](net::TestConnection* connection) {
    connection->AbortOnSend();
  };

  EXPECT_TRUE(client::DoMain(argc, argv, String(), "clang++"));
  EXPECT_TRUE(weak_ptr.expired());
  EXPECT_EQ(1u, send_count);
  EXPECT_EQ(0u, read_count);
  EXPECT_EQ(1u, connect_count);
  EXPECT_EQ(1u, connections_created);
}

TEST_F(ClientTest, CannotReadMessage) {
  const String temp_input = base::CreateTempFile(".cc");
  const char* argv[] = {"clang++", "-c", temp_input.c_str(), nullptr};
  const int argc = 3;

  connect_callback = [&](net::TestConnection* connection) {
    connection->AbortOnRead();
    connection->CallOnSend([&](const net::Connection::Message& message) {
      ASSERT_TRUE(message.HasExtension(proto::Execute::extension));

      const auto& extension = message.GetExtension(proto::Execute::extension);
      EXPECT_FALSE(extension.remote());
      EXPECT_EQ(base::GetCurrentDir(), extension.current_dir());
      ASSERT_TRUE(extension.has_flags());

      const auto& cc_flags = extension.flags();
      ASSERT_TRUE(cc_flags.has_compiler());
      // TODO: check compiler version and path.
      EXPECT_EQ(temp_input, cc_flags.input());
      EXPECT_EQ("c++", cc_flags.language());
      EXPECT_EQ("-emit-obj", cc_flags.action());

      {
        const auto& other = cc_flags.other();
        auto begin = other.begin();
        auto end = other.end();
        EXPECT_NE(end, std::find(begin, end, "-cc1"));
        EXPECT_NE(end, std::find(begin, end, "-triple"));
        EXPECT_NE(end, std::find(begin, end, "-target-cpu"));
        EXPECT_NE(end, std::find(begin, end, "-target-linker-version"));
      }

      {
        const auto& non_cached = cc_flags.non_cached();
        auto begin = non_cached.begin();
        auto end = non_cached.end();
        EXPECT_NE(end, std::find(begin, end, "-main-file-name"));
        EXPECT_NE(end, std::find(begin, end, "-coverage-file"));
        EXPECT_NE(end, std::find(begin, end, "-resource-dir"));
        EXPECT_NE(end, std::find(begin, end, "-internal-isystem"));
        EXPECT_NE(end, std::find(begin, end, "-internal-externc-isystem"));
      }
    });
  };

  EXPECT_TRUE(client::DoMain(argc, argv, String(), "clang++"));
  EXPECT_TRUE(weak_ptr.expired());
  EXPECT_EQ(1u, send_count);
  EXPECT_EQ(1u, read_count);
  EXPECT_EQ(1u, connect_count);
  EXPECT_EQ(1u, connections_created);
}

TEST_F(ClientTest, ReadMessageWithoutStatus) {
  const String temp_input = base::CreateTempFile(".cc");
  const char* argv[] = {"clang++", "-c", temp_input.c_str(), nullptr};
  const int argc = 3;

  EXPECT_TRUE(client::DoMain(argc, argv, String(), "clang++"));
  EXPECT_TRUE(weak_ptr.expired());
  EXPECT_EQ(1u, send_count);
  EXPECT_EQ(1u, read_count);
  EXPECT_EQ(1u, connect_count);
  EXPECT_EQ(1u, connections_created);
}

TEST_F(ClientTest, ReadMessageWithBadStatus) {
  const String temp_input = base::CreateTempFile(".cc");
  const char* argv[] = {"clang++", "-c", temp_input.c_str(), nullptr};
  const int argc = 3;

  connect_callback = [](net::TestConnection* connection) {
    connection->CallOnRead([](net::Connection::Message* message) {
      auto extension = message->MutableExtension(proto::Status::extension);
      extension->set_code(proto::Status::INCONSEQUENT);
    });
  };

  EXPECT_TRUE(client::DoMain(argc, argv, String(), "clang++"));
  EXPECT_TRUE(weak_ptr.expired());
  EXPECT_EQ(1u, send_count);
  EXPECT_EQ(1u, read_count);
  EXPECT_EQ(1u, connect_count);
  EXPECT_EQ(1u, connections_created);
}

TEST_F(ClientTest, SuccessfulCompilation) {
  const String temp_input = base::CreateTempFile(".cc");
  const char* argv[] = {"clang++", "-c", temp_input.c_str(), nullptr};
  const int argc = 3;

  connect_callback = [](net::TestConnection* connection) {
    connection->CallOnRead([](net::Connection::Message* message) {
      auto extension = message->MutableExtension(proto::Status::extension);
      extension->set_code(proto::Status::OK);
    });
  };

  EXPECT_FALSE(client::DoMain(argc, argv, String(), "clang++"));
  EXPECT_TRUE(weak_ptr.expired());
  EXPECT_EQ(1u, send_count);
  EXPECT_EQ(1u, read_count);
  EXPECT_EQ(1u, connect_count);
  EXPECT_EQ(1u, connections_created);
}

TEST_F(ClientTest, FailedCompilation) {
  const String temp_input = base::CreateTempFile(".cc");
  const char* argv[] = {"clang++", "-c", temp_input.c_str(), nullptr};
  const int argc = 3;

  connect_callback = [](net::TestConnection* connection) {
    connection->CallOnRead([](net::Connection::Message* message) {
      auto extension = message->MutableExtension(proto::Status::extension);
      extension->set_code(proto::Status::EXECUTION);
    });
  };

  EXPECT_EXIT(client::DoMain(argc, argv, String(), "clang++"),
              ::testing::ExitedWithCode(1), ".*");
}

TEST_F(ClientTest, DISABLED_MultipleCommands_OneFails) {
  // TODO: implement this.
}

TEST_F(ClientTest, DISABLED_MultipleCommands_Successful) {
  // TODO: implement this.
}

}  // namespace client
}  // namespace dist_clang
