#pragma once

#include <base/testable.h>

#define DECLARE_SINGLETON(Class)                      \
  template <>                                         \
  UniquePtr<Class> base::Singleton<Class>::instance_; \
  template <>                                         \
  std::once_flag base::Singleton<Class>::once_flag_ ;

#define DEFINE_SINGLETON(Class)                            \
  template <>                                              \
  UniquePtr<Class> base::Singleton<Class>::instance_ = {}; \
  template <>                                              \
  std::once_flag base::Singleton<Class>::once_flag_ = {};

namespace dist_clang {
namespace base {

/*
 * Singleton object is created in a lazy manner - on first access. Singleton
 * should be testable, so the testing factory can be set before the creation.
 *
 * Singleton should be used like this:
 *
 *   class Foo { ... };
 *
 *   int main() {
 *     auto& foo = Singleton<Foo>::Get();
 *   }
 *
 * NOTICE: Every singleton'ed class should be explicitly defined in an
 *         appropriate translation unit.
 */
template <class T>
class Singleton : private Testable<T> {
 public:
  static T& Get() {
    std::call_once(once_flag_, [] { instance_ = Testable<T>::Create(); });
    // TODO: post task to |AtExitManager| to destroy this |UniquePtr| on exit.
    // TODO: make possible to friend the Singleton class and make constructors
    //       private.

    return *instance_;
  }

  // TODO: expose |Testable::SetFactory()|.

 private:
  Singleton() = delete;
  Singleton(const Singleton<T>&) = delete;
  Singleton(Singleton<T>&&) = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton& operator=(Singleton&&) = delete;

  static UniquePtr<T> instance_;
  static std::once_flag once_flag_;
};

}  // namespace base
}  // namespace dist_clang
