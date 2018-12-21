// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_DISPATCHER_H
#define JSONRPC_LEAN_DISPATCHER_H

#include "fault.h"
#include "request.h"
#include "response.h"
#include "value.h"

//#if __cplusplus <= 201103L
#include "integer_seq.h"
//namespace std {
//using redi::index_sequence;
//using redi::index_sequence_for;
//} // namespace std
//#endif

#include <functional>
#include <utility>
#include <vector>

#define BOOST_THREAD_VERSION 4
#include <boost/thread/future.hpp>

namespace jsonrpc {

    class MethodWrapper {
    public:
        typedef std::function<Value(const Request::Parameters&)> Method;

        explicit MethodWrapper(Method method) : myMethod(method) {}

        MethodWrapper(const MethodWrapper&) = delete;
        MethodWrapper& operator=(const MethodWrapper&) = delete;

        bool IsHidden() const { return myIsHidden; }
        void SetHidden(bool hidden = true) { myIsHidden = hidden; }

        MethodWrapper& SetHelpText(std::string help) {
            myHelpText = std::move(help);
            return *this;
        }

        const std::string& GetHelpText() const { return myHelpText; }

        template<typename... ParameterTypes>
        MethodWrapper& AddSignature(Value::Type returnType, ParameterTypes... parameterTypes) {
            mySignatures.emplace_back(std::initializer_list < Value::Type > {returnType, parameterTypes...});
            return *this;
        }

        const std::vector<std::vector<Value::Type>>&
            GetSignatures() const { return mySignatures; }

        Value operator()(const Request::Parameters& params) const {
            return myMethod(params);
        }

    private:
        Method myMethod;
        bool myIsHidden = false;
        std::string myHelpText;
        std::vector<std::vector<Value::Type>> mySignatures;
    };

    template<typename> struct ToStdFunction;

    template<typename ReturnType, typename... ParameterTypes>
    struct ToStdFunction < ReturnType(*)(ParameterTypes...) > {
        typedef std::function<ReturnType(ParameterTypes...)> Type;
    };

    template<typename ReturnType, typename T, typename... ParameterTypes>
    struct ToStdFunction < ReturnType(T::*)(ParameterTypes...) > {
        typedef std::function<ReturnType(ParameterTypes...)> Type;
    };

    template<typename ReturnType, typename T, typename... ParameterTypes>
    struct ToStdFunction < ReturnType(T::*)(ParameterTypes...) const > {
        typedef std::function<ReturnType(ParameterTypes...)> Type;
    };

    template<typename MethodType, bool isClass>
    struct StdFunction {};

    template<typename MethodType>
    struct StdFunction < MethodType, false > {
        typedef typename ToStdFunction<MethodType>::Type Type;
    };

    template<typename MethodType>
    struct StdFunction < MethodType, true > {
        typedef typename ToStdFunction <
            decltype(&MethodType::operator()) > ::Type Type;
    };

    class Dispatcher {
    public:
        std::vector<std::string> GetMethodNames(bool includeHidden = false) const {
            std::vector<std::string> names;
            names.reserve(myMethods.size());

            for (auto& method : myMethods) {
                if (includeHidden || !method.second.IsHidden()) {
                    names.emplace_back(method.first);
                }
            }

            return names;
        }

        MethodWrapper& GetMethod(const std::string& name) {
            return myMethods.at(name);
        }

        MethodWrapper& AddMethod(std::string name, MethodWrapper::Method method) {
            auto result = myMethods.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(std::move(name)),
                std::forward_as_tuple(std::move(method)));
            if (!result.second) {
                throw std::invalid_argument(name + ": method already added");
            }
            return result.first->second;
        }
		  
		  

        template<typename MethodType>
        MethodWrapper&
//        typename std::enable_if<!std::is_convertible<MethodType, std::function<Value(const Request::Parameters&)>>::value && 
//										  !std::is_member_pointer<MethodType>::value, MethodWrapper>::type&
        AddMethod(std::string name, MethodType method) {
            //static_assert(!std::is_bind_expression<MethodType>::value,
            //    "Use AddMethod with 3 arguments to add member method");
            typename StdFunction<MethodType, std::is_class<MethodType>::value>::Type function(std::move(method));
            return AddMethodInternal(std::move(name), std::move(function));
        }
		  
		  // for static functions returning futures
		  template<typename ReturnType, typename... ParameterTypes>
        MethodWrapper& AddMethod(std::string name, boost::future<ReturnType>(*fun)(ParameterTypes...)) 
		  {
			  std::function<boost::shared_future<jsonrpc::Value>(ParameterTypes...)> function = 
				  [fun](ParameterTypes&&... params) -> boost::shared_future<jsonrpc::Value> 
				  {
                return fun(std::forward<ParameterTypes>(params)...)
						 .then([](boost::future<ReturnType> f)
						 {
							 return jsonrpc::Value(f.get());
						 })
						 .share();
					};
					
            return AddMethodInternal(std::move(name), std::move(function));
        }
		  
		  // just copyable std::functions
		  template<typename ReturnType, typename... ParameterTypes>
        MethodWrapper& AddAsyncLambda(std::string name, std::function<boost::future<ReturnType>(ParameterTypes...)> fun) 
		  {
			  std::function<boost::shared_future<jsonrpc::Value>(ParameterTypes...)> function = 
				  [fun](ParameterTypes&&... params) -> boost::shared_future<jsonrpc::Value> 
				  {
                return fun(std::forward<ParameterTypes>(params)...)
						 .then([](boost::future<ReturnType> f)
						 {
							 return jsonrpc::Value(f.get());
						 })
						 .share();
					};
					
            return AddMethodInternal(std::move(name), std::move(function));
        }

        template<typename T>
        MethodWrapper& AddMethod(std::string name, Value(T::*method)(const Request::Parameters&), T& instance) {
            return AddMethod(std::move(name), std::bind(method, &instance, std::placeholders::_1));
        }

        template<typename T>
        MethodWrapper& AddMethod(std::string name, Value(T::*method)(const Request::Parameters&) const, T& instance) {
            return AddMethod(std::move(name), std::bind(method, &instance, std::placeholders::_1));
        }

        template<typename ReturnType, typename T, typename... ParameterTypes>
        MethodWrapper& AddMethod(std::string name, ReturnType(T::*method)(ParameterTypes...), T& instance) {
            std::function<ReturnType(ParameterTypes...)> function = [&instance, method](ParameterTypes&&... params) -> ReturnType {
                return (instance.*method)(std::forward<ParameterTypes>(params)...);
            };
            return AddMethodInternal(std::move(name), std::move(function));
        }

        template<typename ReturnType, typename T, typename... ParameterTypes>
        MethodWrapper& AddMethod(std::string name, ReturnType(T::*method)(ParameterTypes...) const, T& instance) {
            std::function<ReturnType(ParameterTypes...)> function = [&instance, method](ParameterTypes&&... params) -> ReturnType {
                return (instance.*method)(std::forward<ParameterTypes>(params)...);
            };
            return AddMethodInternal(std::move(name), std::move(function));
        }
		  
		  // member methods returning futures
		  
		  template<typename ReturnType, typename T, typename... ParameterTypes>
        MethodWrapper& AddMethod(std::string name, boost::future<ReturnType>(T::*method)(ParameterTypes...), 
		  T& instance) 
		  {
			  std::function<boost::shared_future<jsonrpc::Value>(ParameterTypes...)> function = 
				  [&instance, method](ParameterTypes&&... params) -> boost::shared_future<jsonrpc::Value> 
				  {
                return (instance.*method)(std::forward<ParameterTypes>(params)...)
						 .then([](boost::future<ReturnType> f)
						 {
							 return jsonrpc::Value(f.get());	// the outcome must be jsonrpc::Value-compatible
						 })
						 .share();  // convert to SharedFuture which is jsonrpc::Value-compatible
					};
					
            return AddMethodInternal(std::move(name), std::move(function));
        }
		  
		  template<typename T, typename... ParameterTypes>
        MethodWrapper& AddMethod(std::string name, boost::future<void>(T::*method)(ParameterTypes...), 
		  T& instance) 
		  {
			  std::function<boost::shared_future<jsonrpc::Value>(ParameterTypes...)> function = 
				  [&instance, method](ParameterTypes&&... params) -> boost::shared_future<jsonrpc::Value> 
				  {
                return (instance.*method)(std::forward<ParameterTypes>(params)...)
						 .then([](boost::future<void> f)
						 {
							 f.get();
							 return jsonrpc::Value(0);	// the outcome must be jsonrpc::Value-compatible
						 })
						 .share();  // convert to SharedFuture which is jsonrpc::Value-compatible
					};
					
            return AddMethodInternal(std::move(name), std::move(function));
        }
		  
		  template<typename ReturnType, typename T, typename... ParameterTypes>
        MethodWrapper& AddMethod(std::string name, boost::future<ReturnType>(T::*method)(ParameterTypes...) const, 
		  T& instance) 
		  {
			  std::function<boost::shared_future<jsonrpc::Value>(ParameterTypes...)> function = 
				  [&instance, method](ParameterTypes&&... params) -> boost::shared_future<jsonrpc::Value> 
				  {
                return (instance.*method)(std::forward<ParameterTypes>(params)...)
						 .then([](boost::future<ReturnType> f)
						 {
							 return jsonrpc::Value(f.get());
						 })
						 .share();
					};
					
            return AddMethodInternal(std::move(name), std::move(function));
        }

        void RemoveMethod(const std::string& name) {
            myMethods.erase(name);
        }

        Response Invoke(const std::string& name, const Request::Parameters& parameters, const Value& id) const {
            try {
                auto method = myMethods.find(name);
                if (method == myMethods.end()) {
                    throw MethodNotFoundFault("Method not found: " + name);
                }
                return{ method->second(parameters), Value(id) };
            }
            catch (const Fault& fault) {
                return Response(fault.GetCode(), fault.GetString(), Value(id));
            }
				catch (const std::system_error& ex) {
					return Response(ex.code().value(), ex.what(), Value(id), ex.code().category().name());
				}
            catch (const std::exception& ex) {
                return Response(0, ex.what(), Value(id));
            }
            catch (...) {
                return Response(0, "unknown error", Value(id));
            }
        }

    private:
        template<typename ReturnType, typename... ParameterTypes>
        MethodWrapper& AddMethodInternal(std::string name, std::function<ReturnType(ParameterTypes...)> method) {
            return AddMethodInternal(std::move(name), std::move(method), redi::index_sequence_for < ParameterTypes... > {});
        }

        template<typename... ParameterTypes>
        MethodWrapper& AddMethodInternal(std::string name, std::function<void(ParameterTypes...)> method) {
            std::function<Value(ParameterTypes...)> returnMethod = [method](ParameterTypes&&... params) -> Value {
                method(std::forward<ParameterTypes>(params)...);
                return Value();
            };
            return AddMethodInternal(std::move(name), std::move(returnMethod), redi::index_sequence_for < ParameterTypes... > {});
        }

        template<typename ReturnType, typename... ParameterTypes, std::size_t... index>
        MethodWrapper& AddMethodInternal(std::string name, std::function<ReturnType(ParameterTypes...)> method, redi::index_sequence<index...>) {
            MethodWrapper::Method realMethod = [method](const Request::Parameters& params) -> Value {
                if (params.size() != sizeof...(ParameterTypes)) {
                    throw InvalidParametersFault();
                } 
                return method(params[index].AsType<typename std::decay<ParameterTypes>::type>()...);
            };
            return AddMethod(std::move(name), std::move(realMethod));
        }

        std::map<std::string, MethodWrapper> myMethods;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_DISPATCHER_H
