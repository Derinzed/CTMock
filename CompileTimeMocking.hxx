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

#define EXPAND(X) x

#define CREATE_NAME(x) arg // Apply GET_TYPE to each pair of arguments in __VA_ARGS__
#define CREATE_PAIR(x) x arg

#define FOR_EACH_1(what, x) what(x)##1
#define FOR_EACH_2(what, x, ...) what(x)##2, FOR_EACH_1(what, __VA_ARGS__)
#define FOR_EACH_3(what, x, ...) what(x)##3, FOR_EACH_2(what, __VA_ARGS__)
#define FOR_EACH_4(what, x, ...) what(x)##4, FOR_EACH_3(what, __VA_ARGS__)
#define FOR_EACH_5(what, x, ...) what(x)##5, FOR_EACH_4(what, __VA_ARGS__)


#define COUNT_ARGS(_1, _2, _3, _4, _5, N, ...) N
#define COUNT_ARGS_IMPL(...) COUNT_ARGS(__VA_ARGS__, 5,4,3,2,1,0)

#define FOR_EACH_N(N, what, ...) FOR_EACH_##N(what, __VA_ARGS__)
#define FOR_EACH(what, number, ...) FOR_EACH_N(number, what, __VA_ARGS__)

#define GET_ARG_NAMES(...) FOR_EACH(CREATE_NAME, COUNT_ARGS_IMPL(__VA_ARGS__), __VA_ARGS__)
#define GET_ARG_PAIR(...) FOR_EACH(CREATE_PAIR, COUNT_ARGS_IMPL(__VA_ARGS__), __VA_ARGS__)

//#define COMPILE_MOCK(methodName, returnType, ...) static returnType methodName(GET_ARG_PAIR(__VA_ARGS__)) {CTMInvocation<__VA_ARGS__> invocation(methodName, GET_ARG_NAMES(__VA_ARGS__));  return CTMHandler::getInstance().invokeMock<returnType, ##__VA_ARGS__>(invocation);}

#define COMPILE_MOCK(methodName, returnType, ...) static returnType methodName(GET_ARG_PAIR(__VA_ARGS__)) {CTMInvocation<__VA_ARGS__> invocation(methodName, GET_ARG_NAMES(__VA_ARGS__));  return CTMHandler::getInstance().invokeMock<returnType, ##__VA_ARGS__>(invocation);}


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
		IsEqualMatcher(int pos, T match) : mValueToMatch{ match }, CTMMatcher_Base(pos){}
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
		CTMMatcherStatement_Base(void* method, std::vector<CTMMatcher_Base*> matchers) : mMethod{method}, mMatchers{matchers}{}
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

		void setMatcherDefaultReturnValue() {

		}

		//T - StatementType
		//S - Return type
		//Ts - pack of CTMMatchers
		template<class T, class S, class... Ts>
		void setupMatcherStatement(void* func, S returnValue, Ts&&... matchers) {
			//if the method has not been set up for mocking, throw an error.
			//if (mRegisteredFuncs.find(func) == mRegisteredFuncs.end()) {
			//	return;
			//}

			//std::vector<CTMMatcher_Base*> matcherVect;
			//(matcherVect.emplace_back((CTMMatcher_Base*)matchers), ...);
			//CTMMatcherStatement_Base* matcherStatement = new T(func, matcherVect);

			//mMatcherHandler.registerMatcherStatement<S>(matcherStatement, "NoName");
			//CTMReturn<S>& ctmReturn = mMatcherHandler.getReturnForStatement<S>(matcherStatement);
			//ctmReturn.setDefaultValue(returnValue);

			setupNamedMatcherStatement<T, S, Ts...>(std::string("NoName"), func, returnValue, std::forward<Ts>(matchers)...);
		}
		//T - StatementType
		//S - Return type
		//Ts - pack of CTMMatchers
		template<class T, class S, class... Ts>
		void setupNamedMatcherStatement(std::string& name, void* func, S returnValue,  Ts&&... matchers) {
			//if the method has not been set up for mocking, throw an error.
			if (mRegisteredFuncs.find(func) == mRegisteredFuncs.end()) {
				return;
			}

			std::vector<CTMMatcher_Base*> matcherVect;
			(matcherVect.emplace_back((CTMMatcher_Base*)matchers), ...);
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
		std::vector<T*> getMatchingStatements(std::string& name) {
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

	//base class used to relate all CTM classess
	class MockObject {
	public:

		CTMHandler registrar = CTMHandler::getInstance();

	private:
		int mockObjectsCreated = 0;
	};

}