#include <client/command.h>

#include <proto/remote.pb.h>

#include <clang/Basic/Diagnostic.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/DriverDiagnostic.h>
#include <clang/Driver/Options.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/Option/Arg.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

namespace dist_clang {
namespace client {

// static
bool Command::GenerateFromArgs(int argc, const char* const raw_argv[],
                               List<Command>& commands) {
  using namespace clang;
  using namespace clang::driver;
  using namespace llvm;
  using DiagPrinter = TextDiagnosticPrinter;

  SmallVector<const char*, 256> argv;
  SpecificBumpPtrAllocator<char> arg_allocator;
  auto&& arg_array = ArrayRef<const char*>(raw_argv, argc);
  llvm::sys::Process::GetArgumentVector(argv, arg_array, arg_allocator);

  llvm::InitializeAllTargets();  // Multiple calls per program are allowed.

  IntrusiveRefCntPtr<DiagnosticOptions> diag_opts = new DiagnosticOptions;
  DiagPrinter* diag_client = new DiagPrinter(llvm::errs(), &*diag_opts);
  IntrusiveRefCntPtr<DiagnosticIDs> diag_id(new DiagnosticIDs());
  IntrusiveRefCntPtr<clang::DiagnosticsEngine> diags;
  SharedPtr<clang::driver::Compilation> compilation;

  diags = new DiagnosticsEngine(diag_id, &*diag_opts, diag_client);

  std::string path = argv[0];
  Driver driver(path, llvm::sys::getDefaultTargetTriple(), "a.out", *diags);
  compilation.reset(driver.BuildCompilation(argv));

  if (!compilation) {
    return false;
  }

  SharedPtr<opt::OptTable> opts(createDriverOptTable());
  bool result = false;
  const auto& jobs = compilation->getJobs();
  for (auto& job : jobs) {
    if (job->getKind() == Job::CommandClass) {
      result = true;

      auto command = static_cast<clang::driver::Command*>(job);
      auto arg_begin = command->getArguments().begin();
      auto arg_end = command->getArguments().end();

      const unsigned included_flags_bitmask = options::CC1Option;
      unsigned missing_arg_index, missing_arg_count;
      commands.emplace_back(std::move(
          Command(opts->ParseArgs(arg_begin, arg_end, missing_arg_index,
                                  missing_arg_count, included_flags_bitmask),
                  compilation, opts)));
      const auto& arg_list = commands.back().arg_list_;

      // Check for missing argument error.
      if (missing_arg_count) {
        diags->Report(diag::err_drv_missing_argument)
            << arg_list->getArgString(missing_arg_index) << missing_arg_count;
        return false;
      }

      // Issue errors on unknown arguments.
      for (auto it = arg_list->filtered_begin(options::OPT_UNKNOWN),
                end = arg_list->filtered_end();
           it != end; ++it) {
        diags->Report(diag::err_drv_unknown_argument)
            << (*it)->getAsString(*arg_list);
        return false;
      }
    }
  }

  return result;
}

void Command::FillFlags(proto::Flags* flags) const {
  if (flags) {
    flags->Clear();

    llvm::opt::ArgStringList non_cached_list, other_list;

    for (const auto& arg : *arg_list_) {
      using namespace clang::driver::options;

      if (arg->getOption().getKind() == llvm::opt::Option::InputClass) {
        flags->set_input(arg->getValue());
      } else if (arg->getOption().matches(OPT_add_plugin)) {
        flags->add_other(arg->getSpelling());
        flags->add_other(arg->getValue());
        flags->mutable_compiler()->add_plugins()->set_name(arg->getValue());
      } else if (arg->getOption().matches(OPT_emit_obj)) {
        flags->set_action(arg->getSpelling());
      } else if (arg->getOption().matches(OPT_E)) {
        flags->set_action(arg->getSpelling());
      } else if (arg->getOption().matches(OPT_dependency_file)) {
        flags->set_deps_file(arg->getValue());
      } else if (arg->getOption().matches(OPT_load)) {
        // FIXME: maybe claim this type of args right after generation?
        continue;
      } else if (arg->getOption().matches(OPT_mrelax_all)) {
        flags->add_cc_only(arg->getSpelling());
      } else if (arg->getOption().matches(OPT_o)) {
        flags->set_output(arg->getValue());
      } else if (arg->getOption().matches(OPT_x)) {
        flags->set_language(arg->getValue());
      }

      // Non-cacheable flags.
      // NOTICE: we should be very cautious here, since the local compilations
      // are
      //         performed on a non-preprocessed file, but the result is saved
      //         using the hash from a preprocessed file.
      else if (arg->getOption().matches(OPT_coverage_file) ||
               arg->getOption().matches(OPT_fdebug_compilation_dir) ||
               arg->getOption().matches(OPT_ferror_limit) ||
               arg->getOption().matches(OPT_include) ||
               arg->getOption().matches(OPT_internal_externc_isystem) ||
               arg->getOption().matches(OPT_internal_isystem) ||
               arg->getOption().matches(OPT_isysroot) ||
               arg->getOption().matches(OPT_main_file_name) ||
               arg->getOption().matches(OPT_MF) ||
               arg->getOption().matches(OPT_MMD) ||
               arg->getOption().matches(OPT_MT) ||
               arg->getOption().matches(OPT_resource_dir)) {
        arg->render(*arg_list_, non_cached_list);
      }

      // By default all other flags are cacheable.
      else {
        arg->render(*arg_list_, other_list);
      }
    }

    for (const auto& value : non_cached_list) {
      flags->add_non_cached(value);
    }
    for (const auto& value : other_list) {
      flags->add_other(value);
    }
  }
}

String Command::RenderAllArgs() const {
  String result;

  for (const auto& arg : *arg_list_) {
    result += String(" ") + arg->getAsString(*arg_list_);
  }

  return result.substr(1);
}

}  // namespace client
}  // namespace dist_clang
