#include <variant>
#include <queue>
#include <assert.h>
#include <tuple>
#include <any>
#include <list>
#include <map>
#include <string>
#include <optional>
#include <type_traits>
#include <exception>
#include <utility>

//#define CTM_CLASS(className) class className{ public: 
//#define COMPILE_MOCK(methodName, returnType, ...) static returnType methodName(GET_ARG_PAIR(__VA_ARGS__)) {CTMInvocation<__VA_ARGS__> invocation(methodName, GET_ARG_NAMES(__VA_ARGS__));  return CTMHandler::getInstance().invokeMock<returnType, ##__VA_ARGS__>(invocation);}
//#define COMPILE_MOCK_VOID(methodName, ...) static void methodName(GET_ARG_PAIR(__VA_ARGS__)) {CTMInvocation<__VA_ARGS__> invocation(methodName, GET_ARG_NAMES(__VA_ARGS__));  return CTMHandler::getInstance().invokeMock<void, ##__VA_ARGS__>(invocation);}
//#define CTM_CLASS_END };

#define CM0(methodName, returnType) static returnType methodName() {CTMInvocation<> invocation(methodName);  return CTMHandler::getInstance().invokeMock<returnType>(invocation);}
#define CM1(methodName, returnType, argType1) static returnType methodName(argType1 arg1) {CTMInvocation<argType1> invocation(methodName, arg1);  return CTMHandler::getInstance().invokeMock<returnType, argType1>(invocation);}
#define CM2(methodName, returnType, argType1, argType2) static returnType methodName(argType1 arg1, argType2 arg2) {CTMInvocation<argType1, argType2> invocation(methodName, arg1, arg2);  return CTMHandler::getInstance().invokeMock<returnType, argType1, argType2>(invocation);}
#define CM3(methodName, returnType, argType1, argType2, argType3) static returnType methodName(argType1 arg1, argType2 arg2, argType3 arg3) {CTMInvocation<argType1, argType2, argType3> invocation(methodName, arg1, arg2, arg3);  return CTMHandler::getInstance().invokeMock<returnType, argType1, argType2, argType3>(invocation);}
#define CM4(methodName, returnType, argType1, argType2, argType3, argType4) static returnType methodName(argType1 arg1, argType2 arg2, argType3 arg3, argType4 arg4) {CTMInvocation<argType1, argType2, argType3, argType4> invocation(methodName, arg1, arg2, arg3, arg4);  return CTMHandler::getInstance().invokeMock<returnType, argType1, argType2, argType3, argType4>(invocation);}
#define CM5(methodName, returnType, argType1, argType2, argType3, argType4, argType5) static returnType methodName(argType1 arg1, argType2 arg2, argType3 arg3, argType4 arg4, argType5 arg5) {CTMInvocation<argType1, argType2, argType3, argType4, argType5> invocation(methodName, arg1, arg2, arg3, arg4, arg5);  return CTMHandler::getInstance().invokeMock<returnType, argType1, argType2, argType3, argType4, argType5>(invocation);}
#define GET_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME


#define CTM_CLASS(className) class className{ public: 
#define COMPILE_MOCK(methodName, returnType, ...) GET_MACRO(__VA_ARGS__ __VA_OPT__(,) CM5, CM4, CM3, CM2, CM1, CM0)(methodName, returnType __VA_OPT__(,) __VA_ARGS__ )
#define CTM_CLASS_END };

#pragma once
#define MOCK_EXPORTS _declspec(dllexport)
#define CTM_MOCK CTMHandler::getInstance()

template<typename T, typename = void>
struct is_container : std::false_type {};

template <typename T>
struct is_container<T, std::void_t<typename T::value_type>> : std::true_type {};

template <typename T>
inline constexpr bool is_container_v = is_container<T>::value;


//Will impliment generic, compile time, mocking object and mocking utilities
//designed for use with NNS_CPP objects

//NOTES:
//compile time mock should work to only support the main class's method implimentation. The defined method will execute, but we must support it's ability to return differently.
//

//Example Use cases
//Create a mock that returns nothing
//

namespace CompileTimeMocking {

	class CTMException : public std::exception {
	public:
		CTMException(const std::string& msg) : mMessage(msg) {

		}
		std::string what() {
			return mMessage;
		}
	private:
		std::string mMessage;
	};

	//class to be used in the future for mocks to
	//perform actions upon invokation.
	//NOTE: Should be tied to matching statements
	//NOTE: Allow performing of action based on statement return
	class CTMAction {
	public:

	};

	template<class T>
	class CTMReturn {
	public:
		T getReturn() {
			if (returnQueue.empty()) {
				return defaultValue;
			}
			T val = std::any_cast<T>(returnQueue.front());
			returnQueue.pop();
			return val;
		}
		void setDefaultValue(T value) {
			defaultValue = value;
		}
		std::queue<T> returnQueue;
		T defaultValue;
	};

	template<>
	class CTMReturn<void> {
	public:
		void getReturn() {
			return;
		}
		void setDefaultValue(std::any arg) {

		}
	};

	//base abstract interface to be used by all matchers
	//defines the contract that makes up a matcher
	//allows all matchers to be contained within a single vector within the handler
	class CTMMatcher_Base {
	public:
		CTMMatcher_Base(int pos) : mArgumentPos{ pos } {}
		int getArgumentPos() {
			return mArgumentPos;
		}
		virtual bool resolveMatch(const std::any& arg) = 0;
	private:
		int mArgumentPos;
	};

	//default base matcher for all registered mocked methods
	template<class T>
	class DefaultMatcher : public CTMMatcher_Base {
	public:
		DefaultMatcher(int pos) : CTMMatcher_Base(pos) {}
		void getArgumentPos() {
			return 0;
		}
		bool resolveMatch(const std::any& arg) override {
			return true;
		}
	private:
		T mValueToMatch;
	};

	//matcher that compares exact values
	template<class T>
	class IsEqualMatcher : public CTMMatcher_Base {
	public:
		IsEqualMatcher(int pos, T match) : mValueToMatch{ match }, CTMMatcher_Base(pos) {}
		int getArgumentPos() {
			return mArgumentPos;
		}
		bool resolveMatch(const std::any& arg) override {
			if (arg.type() != typeid(T))
				return false;
			return mValueToMatch == std::any_cast<T>(arg);
		}
	private:
		T mValueToMatch;
	};

	class CTMMatcherStatement_Base {
	public:
		CTMMatcherStatement_Base(void* method, std::vector<CTMMatcher_Base*> matchers) : mMethod{ method }, mMatchers{ matchers } {}
		bool templateResolutionMethod(std::vector<std::any>& arg) {
			bool result = resolveStatement(arg);

			if (result == true) {
				matchMade();
			}
			return result;
		}
		virtual bool resolveStatement(std::vector<std::any>& arg) = 0;
		virtual void* getMethod() {
			return mMethod;
		}
		virtual std::string getStatementName() {
			return mStatementName;
		}
		virtual void setStatementName(const std::string& name) {
			mStatementName = name;
		}
		std::vector<CTMMatcher_Base*> getMatchers() {
			return mMatchers;
		}
		std::vector<CTMMatcher_Base*> getMatchersForArgPos(int pos) {
			std::vector<CTMMatcher_Base*> result;
			for (auto& matcher : mMatchers) {
				if (matcher->getArgumentPos() == pos) {
					result.push_back(matcher);
				}
			}
			return result;
		}
		void matchMade() {
			mMatchesMade++;
		}
		int getMatchesMade() {
			return mMatchesMade;
		}
	private:
		void* mMethod;
		int mMatchesMade = 0;
		std::vector<CTMMatcher_Base*> mMatchers;
		std::string mStatementName = "Default";
	};

	//The default matcher statement exists on all mocks.
	//It ensures there is something to return
	class DefaultMatcherStatement : public CTMMatcherStatement_Base {
	public:
		DefaultMatcherStatement(void* method, std::vector<CTMMatcher_Base*> matchers) : CTMMatcherStatement_Base(method, matchers) {}
		bool resolveStatement(std::vector<std::any>& arg) override {
			return true;
		}
	};

	class AllTrueMatcherStatement : public CTMMatcherStatement_Base {
	public:
		AllTrueMatcherStatement(void* method, std::vector<CTMMatcher_Base*> matchers) : CTMMatcherStatement_Base(method, matchers) {}
		bool resolveStatement(std::vector<std::any>& arg) override {
			if (arg.size() == 0) {
				return false;
			}
			for (int i = 0; i < arg.size(); i++) {
				std::vector<CTMMatcher_Base*> matchers = getMatchersForArgPos(i);
				for (auto matcher : matchers) {
					if (matcher->resolveMatch(arg[i]) == false) {
						return false;
					}
				}
			}
			return true;
		}
	};

	class CTMMatcherHandler {
	public:
		CTMMatcherHandler() {}
		~CTMMatcherHandler() {
			for (auto statement : matcherStatements) {
				delete statement.first;
			}
		}
		template<class T>
		void registerMatcherStatement(CTMMatcherStatement_Base* statement, const std::string& name) {
			std::pair<CTMMatcherStatement_Base*, CTMReturn<T>> pair = std::pair<CTMMatcherStatement_Base*, CTMReturn<T>>(statement, CTMReturn<T>());
			pair.first->setStatementName(name);
			matcherStatements.push_front(pair);
		}

		std::vector<CTMMatcherStatement_Base*> getMatcherStatementsForMethod(void* func) {
			std::vector<CTMMatcherStatement_Base*> result;
			for (auto statement : matcherStatements) {
				if (statement.first->getMethod() == func)
					result.push_back(statement.first);
			}
			return result;
		}

		std::vector<CTMMatcherStatement_Base*> getMatcherStatementsForName(const std::string& name) {
			std::vector<CTMMatcherStatement_Base*> result;
			for (auto statement : matcherStatements) {
				if (statement.first->getStatementName() == name)
					result.push_back(statement.first);
			}
			return result;
		}

		template<class T>
		CTMReturn<T>& getReturnForStatement(CTMMatcherStatement_Base* state) {
			for (auto& statement : matcherStatements) {
				if (statement.first == state)
					return std::any_cast<CTMReturn<T>&>(statement.second);
			}
		}
		std::list<std::pair<CTMMatcherStatement_Base*, std::any>> matcherStatements; //- New version to allow for all kinds of returns
		//std::list<std::pair<CTMMatcherStatement_Base*, std::variant<CTMReturn<int>, CTMReturn<double>, CTMReturn<std::string>>>> matcherStatements;
	};

	//class... Args - The arg types used in the invocation.
	template<class... Args>
	class CTMInvocation {
	public:
		CTMInvocation(void* func, Args... args) : invokedMethod{ func } {
			(invokedArguments.emplace_back(args), ...);
		}

		void* invokedMethod;
		std::vector<std::any> invokedArguments;
	};

	//singleton class that is responsible for managing and processing mock invocations
	class CTMInvocationHandler {
	public:
		CTMInvocationHandler() {}

		template<class T, class... Ts>
		T processInvocation(CTMMatcherHandler& matcherHandler, CTMInvocation<Ts...>& invocation) {

			//get all statements tied to the calling method
			//test each statement
			//the first statement to resolve true determines the return
			std::vector<CTMReturn<T>*> result;
			std::vector<CTMMatcherStatement_Base*> statements = matcherHandler.getMatcherStatementsForMethod(invocation.invokedMethod);
			for (auto statement : statements) {
				if (statement->templateResolutionMethod(invocation.invokedArguments) == true) {
					CTMReturn<T>& res = matcherHandler.getReturnForStatement<T>(statement);
					result.push_back(&res);
				}
			}
			if (result.size() == 0) {
				throw CTMException("Error, no statements were found that resolved to true.  This is an error since all mocks should at least have a default statement. Did you forget to set up a mock?");
			}
			return result.at(0)->getReturn();

		}
		template<class... Ts>
		std::vector<std::any> convertArgsToVector(Ts... args) {
			std::vector<std::any> vec;
			(vec.emplace_back(args), ...);
			return vec;
		}
	};

	//Singleton class that is responsible for managing CTM Mocks and setting up matchers and returns
	class CTMHandler {
	public:
		static CTMHandler& getInstance() {
			static CTMHandler instance;
			return instance;
		}

		template<class T>
		typename std::enable_if<std::is_void<T>::value, void>::type
			setupForMocking(void* func) {
			if (mRegisteredFuncs.find(func) != mRegisteredFuncs.end()) {
				return;
			}
			//The default matcher statement ensures that any call to the mock will return at least a base default value
			DefaultMatcherStatement* defaultStatement = new DefaultMatcherStatement(func, std::vector<CTMMatcher_Base*>());
			mMatcherHandler.registerMatcherStatement<T>(defaultStatement, "Default");

			mRegisteredFuncs[func] = 0;
		}
		template<class T>
		typename std::enable_if<!std::is_void<T>::value, void>::type
			setupForMocking(void* func, T defaultValue) {
			if (mRegisteredFuncs.find(func) != mRegisteredFuncs.end()) {
				return;
			}

			//The default matcher statement ensures that any call to the mock will return at least a base default value
			DefaultMatcherStatement* defaultStatement = new DefaultMatcherStatement(func, std::vector<CTMMatcher_Base*>());
			mMatcherHandler.registerMatcherStatement<T>(defaultStatement, "Default");
			//setup the returns default value
			CTMReturn<T>& ctmReturn = mMatcherHandler.getReturnForStatement<T>(defaultStatement);
			ctmReturn.setDefaultValue(defaultValue);

			mRegisteredFuncs[func] = 0;
		}

		template<class T>
		void setupReturnQueue(void* func, T returnVal) {

			auto statements = mMatcherHandler.getMatcherStatementsForMethod(func);

			for (auto statement : statements) {
				if (DefaultMatcherStatement* defaultStatement = dynamic_cast<DefaultMatcherStatement*>(statement)) {
					CTMReturn<T>& ctmReturn = mMatcherHandler.getReturnForStatement<T>(defaultStatement);
					ctmReturn.returnQueue.push(returnVal);
					return;
				}
			}
		}

		template<class T>
		void setupReturnQueueForStatement(const std::string& statementName, void* func, T returnVal) {

			auto statements = mMatcherHandler.getMatcherStatementsForMethod(func);

			for (auto statement : statements) {
				if (statement->getStatementName() == statementName) {
					CTMReturn<T>& ctmReturn = mMatcherHandler.getReturnForStatement<T>(statement);
					ctmReturn.returnQueue.push(returnVal);
					return;
				}
			}
		}

		void setMatcherDefaultReturnValue() {

		}

		//T - StatementType
		//S - Return type
		//Ts - pack of CTMMatchers
		template<class T, class S, class... Ts>
		void setupMatcherStatement(void* func, S returnValue, Ts&&... matchers) {

			setupNamedMatcherStatement<T, S, Ts...>(std::string("NoName"), func, returnValue, std::forward<Ts>(matchers)...);
		}
		//T - StatementType
		//Ts - pack of CTMMatchers
		template<class T, class S, class... Ts>
		std::enable_if_t<std::is_void_v<S>, void>
	    setupNamedMatcherStatement(const std::string& name, void* func, Ts&&... matchers) {
			//if the method has not been set up for mocking, throw an error.
			if (mRegisteredFuncs.find(func) == mRegisteredFuncs.end()) {
				return;
			}

			std::vector<CTMMatcher_Base*> matcherVect;
			(matcherVect.emplace_back(new std::decay_t<Ts>(matchers)), ...);
			CTMMatcherStatement_Base* matcherStatement = new T(func, matcherVect);

			mMatcherHandler.registerMatcherStatement<S>(matcherStatement, name);
			CTMReturn<S>& ctmReturn = mMatcherHandler.getReturnForStatement<S>(matcherStatement);
		}
		//T - StatementType
		//S - Return type
		//Ts - pack of CTMMatchers
		template<class T, class S, class... Ts>
		void setupNamedMatcherStatement(const std::string& name, void* func, S returnValue, Ts&&... matchers) {
			//if the method has not been set up for mocking, throw an error.
			if (mRegisteredFuncs.find(func) == mRegisteredFuncs.end()) {
				return;
			}

			std::vector<CTMMatcher_Base*> matcherVect;
			(matcherVect.emplace_back(new std::decay_t<Ts>(matchers)), ...);
			CTMMatcherStatement_Base* matcherStatement = new T(func, matcherVect);

			mMatcherHandler.registerMatcherStatement<S>(matcherStatement, name);
			CTMReturn<S>& ctmReturn = mMatcherHandler.getReturnForStatement<S>(matcherStatement);
			ctmReturn.setDefaultValue(returnValue);
		}


		//T - return type
		template <class T, class... Ts>
		T invokeMock(CTMInvocation<Ts...> invocation) {
			mTrackRegisteredFuncCalls(invocation.invokedMethod);
			return mInvocationHandler.processInvocation<T>(mMatcherHandler, invocation);
		}

		int getInvocationCount(void* func) {
			if (mRegisteredFuncs.find(func) == mRegisteredFuncs.end()) {
				return 0;
			}
			return mRegisteredFuncs.at(func);
		}

		template<class T>
		std::vector<T*> getMatchingStatements(void* func) {
			std::vector<T*> returnVect;
			auto statements = mMatcherHandler.getMatcherStatementsForMethod(func);
			for (auto state : statements) {
				T* derived = NULL;
				if (derived = dynamic_cast<T*>(state)) {
					returnVect.push_back(derived);
				}
			}
			return returnVect;
		}
		template<class T>
		std::vector<T*> getMatchingStatements(const std::string& name) {
			std::vector<T*> returnVect;
			auto statements = mMatcherHandler.getMatcherStatementsForName(name);
			for (auto state : statements) {
				T* derived = NULL;
				if (derived = dynamic_cast<T*>(state)) {
					returnVect.push_back(derived);
				}
			}
			return returnVect;
		}

		void cleanupMocks() {
			mRegisteredFuncs.clear();
			mMatcherHandler.matcherStatements.clear();
		}

	private:
		CTMHandler() {};

		void mTrackRegisteredFuncCalls(void* func) {
			if (mRegisteredFuncs.find(func) == mRegisteredFuncs.end()) {
				mRegisteredFuncs[func] = 1;
			}
			else {
				mRegisteredFuncs.at(func)++;
			}
		}

		std::map<void*, int> mRegisteredFuncs;

		CTMInvocationHandler mInvocationHandler = CTMInvocationHandler();
		CTMMatcherHandler mMatcherHandler = CTMMatcherHandler();
	};

	//a wrapper class designed to be a user friendly API
	//Functionality should wrap the CTMHandler only
	//no functionality should be added that isn't directly supported from the handler
	class CTM2 {
	public:
	};

	//base class used to relate all CTM classess
	class MockObject {
	public:

		CTMHandler registrar = CTMHandler::getInstance();

	private:
		int mockObjectsCreated = 0;
	};

}
