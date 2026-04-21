#include <bits/stdc++.h>
using namespace std;

class RefCellError : public std::runtime_error {
public:
    explicit RefCellError(const std::string& message) : std::runtime_error(message) {}
    virtual ~RefCellError() = default;
};

class BorrowError : public RefCellError {
public:
    explicit BorrowError(const std::string& message) : RefCellError(message) {}
};

class BorrowMutError : public RefCellError {
public:
    explicit BorrowMutError(const std::string& message) : RefCellError(message) {}
};

class DestructionError : public RefCellError {
public:
    explicit DestructionError(const std::string& message) : RefCellError(message) {}
};

template <typename T>
class RefCell {
private:
    struct State {
        mutable int imm_cnt{0};
        mutable bool mut_borrowed{false};
    };

    T value;
    shared_ptr<State> state;

    void inc_imm() const { ++state->imm_cnt; }
    void dec_imm() const { --state->imm_cnt; }
    void set_mut(bool v) const { state->mut_borrowed = v; }

public:
    class Ref;
    class RefMut;

    explicit RefCell(const T& initial_value) : value(initial_value), state(make_shared<State>()) {}
    explicit RefCell(T&& initial_value) : value(std::move(initial_value)), state(make_shared<State>()) {}

    RefCell(const RefCell&) = delete;
    RefCell& operator=(const RefCell&) = delete;
    RefCell(RefCell&&) = delete;
    RefCell& operator=(RefCell&&) = delete;

    class Ref {
    private:
        const T* ptr{nullptr};
        shared_ptr<State> st;
    public:
        Ref() = default;
        Ref(const T* p, const shared_ptr<State>& s) : ptr(p), st(s) { if(st) ++st->imm_cnt; }
        ~Ref(){ if(st){ --st->imm_cnt; } }

        const T& operator*() const { return *ptr; }
        const T* operator->() const { return ptr; }

        Ref(const Ref& other) : ptr(other.ptr), st(other.st) { if(st) ++st->imm_cnt; }
        Ref& operator=(const Ref& other){ if(this==&other) return *this; if(st) --st->imm_cnt; ptr=other.ptr; st=other.st; if(st) ++st->imm_cnt; return *this; }
        Ref(Ref&& other) noexcept : ptr(other.ptr), st(std::move(other.st)) { other.ptr=nullptr; }
        Ref& operator=(Ref&& other) noexcept { if(this==&other) return *this; if(st) --st->imm_cnt; ptr=other.ptr; st=std::move(other.st); other.ptr=nullptr; return *this; }
    };

    class RefMut {
    private:
        T* ptr{nullptr};
        shared_ptr<State> st;
    public:
        RefMut() = default;
        RefMut(T* p, const shared_ptr<State>& s) : ptr(p), st(s) {}
        ~RefMut(){ if(st) st->mut_borrowed=false; }

        T& operator*(){ return *ptr; }
        T* operator->(){ return ptr; }

        RefMut(const RefMut&) = delete;
        RefMut& operator=(const RefMut&) = delete;
        RefMut(RefMut&& other) noexcept : ptr(other.ptr), st(std::move(other.st)) { other.ptr=nullptr; }
        RefMut& operator=(RefMut&& other) noexcept { if(this==&other) return *this; if(st) st->mut_borrowed=false; ptr=other.ptr; st=std::move(other.st); other.ptr=nullptr; return *this; }
    };

    Ref borrow() const {
        if(state->mut_borrowed) throw BorrowError("immutable borrow while mutable exists");
        return Ref(&value, state);
    }

    optional<Ref> try_borrow() const {
        if(state->mut_borrowed) return nullopt;
        return Ref(&value, state);
    }

    RefMut borrow_mut(){
        if(state->mut_borrowed || state->imm_cnt>0) throw BorrowMutError("mutable borrow conflict");
        state->mut_borrowed=true;
        return RefMut(&value, state);
    }

    optional<RefMut> try_borrow_mut(){
        if(state->mut_borrowed || state->imm_cnt>0) return nullopt;
        state->mut_borrowed=true;
        return RefMut(&value, state);
    }

    ~RefCell(){
        if(state){
            if(state->mut_borrowed || state->imm_cnt!=0){
                throw DestructionError("RefCell destroyed while borrowed");
            }
        }
    }
};

int main(){
    // No I/O needed; behavior tested by judge harness.
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    return 0;
}

