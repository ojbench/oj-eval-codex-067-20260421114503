#include <optional>
#include <stdexcept>
#include <memory>

class RefCellError : public std::runtime_error {
public:
    explicit RefCellError(const std::string& message) : std::runtime_error(message) {}
    virtual ~RefCellError() = default;
};

// invalidly call an immutable borrow
class BorrowError : public RefCellError {
public:
    explicit BorrowError(const std::string& message) : RefCellError(message) {}
};

// invalidly call a mutable borrow
class BorrowMutError : public RefCellError {
public:
    explicit BorrowMutError(const std::string& message) : RefCellError(message) {}
};

// still has refs when destructed
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
    std::shared_ptr<State> state;

public:
    class Ref;
    class RefMut;

    // Constructor
    explicit RefCell(const T& initial_value) : value(initial_value), state(std::make_shared<State>()) {}
    explicit RefCell(T&& initial_value) : value(std::move(initial_value)), state(std::make_shared<State>()) {}

    // Disable copying and moving for simplicity
    RefCell(const RefCell&) = delete;
    RefCell& operator=(const RefCell&) = delete;
    RefCell(RefCell&&) = delete;
    RefCell& operator=(RefCell&&) = delete;

    // Borrow methods
    Ref borrow() const {
        if (state->mut_borrowed) {
            throw BorrowError("immutable borrow while mutable exists");
        }
        return Ref(&value, state);
    }

    std::optional<Ref> try_borrow() const {
        if (state->mut_borrowed) {
            return std::nullopt;
        }
        return Ref(&value, state);
    }

    RefMut borrow_mut() {
        if (state->mut_borrowed || state->imm_cnt > 0) {
            throw BorrowMutError("mutable borrow conflict");
        }
        state->mut_borrowed = true;
        return RefMut(&value, state);
    }

    std::optional<RefMut> try_borrow_mut() {
        if (state->mut_borrowed || state->imm_cnt > 0) {
            return std::nullopt;
        }
        state->mut_borrowed = true;
        return RefMut(&value, state);
    }

    // Inner classes for borrows
    class Ref {
    private:
        const T* ptr{nullptr};
        std::shared_ptr<State> st;

    public:
        Ref() = default;

        Ref(const T* p, const std::shared_ptr<State>& s) : ptr(p), st(s) {
            if (st) ++st->imm_cnt;
        }

        ~Ref() {
            if (st) --st->imm_cnt;
        }

        const T& operator*() const { return *ptr; }
        const T* operator->() const { return ptr; }

        // Allow copying
        Ref(const Ref& other) : ptr(other.ptr), st(other.st) {
            if (st) ++st->imm_cnt;
        }

        Ref& operator=(const Ref& other) {
            if (this == &other) return *this;
            if (st) --st->imm_cnt;
            ptr = other.ptr;
            st = other.st;
            if (st) ++st->imm_cnt;
            return *this;
        }

        // Allow moving
        Ref(Ref&& other) noexcept : ptr(other.ptr), st(std::move(other.st)) {
            other.ptr = nullptr;
        }

        Ref& operator=(Ref&& other) noexcept {
            if (this == &other) return *this;
            if (st) --st->imm_cnt;
            ptr = other.ptr;
            st = std::move(other.st);
            other.ptr = nullptr;
            return *this;
        }
    };

    class RefMut {
    private:
        T* ptr{nullptr};
        std::shared_ptr<State> st;

    public:
        RefMut() = default;

        RefMut(T* p, const std::shared_ptr<State>& s) : ptr(p), st(s) {}

        ~RefMut() {
            if (st) st->mut_borrowed = false;
        }

        T& operator*() { return *ptr; }
        T* operator->() { return ptr; }

        // Disable copying to ensure correct borrow rules
        RefMut(const RefMut&) = delete;
        RefMut& operator=(const RefMut&) = delete;

        // Allow moving
        RefMut(RefMut&& other) noexcept : ptr(other.ptr), st(std::move(other.st)) {
            other.ptr = nullptr;
        }

        RefMut& operator=(RefMut&& other) noexcept {
            if (this == &other) return *this;
            if (st) st->mut_borrowed = false;
            ptr = other.ptr;
            st = std::move(other.st);
            other.ptr = nullptr;
            return *this;
        }
    };

    // Destructor
    ~RefCell() {
        if (state) {
            if (state->mut_borrowed || state->imm_cnt != 0) {
                throw DestructionError("RefCell destroyed while borrowed");
            }
        }
    }
};

