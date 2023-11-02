#pragma once

#include <memory>



template<class T>using Unique=std::unique_ptr<T>;
template<class T>using Shared=std::shared_ptr<T>;
template<class T>using SharedConst=std::shared_ptr<const T>;
template<class T>using Weak=std::weak_ptr<T>;