#include "Wren.h"

namespace wrenly {

Wren::Wren()
:   vm_( nullptr ),
    refCount_( nullptr ) {
    refCount_ = new unsigned;
    *refCount_ = 1u;
    
    // wren set up goes here....
}

Wren::Wren( const Wren& other )
:   vm_( other.vm_ ),
    refCount_( other.refCount_ ) {
    retain_();
}

Wren& Wren::operator=( const Wren& rhs ) {
    release_();
    vm_         = rhs.vm_;
    refCount_   = rhs.refCount_;
    retain_();
    return *this;
}

Wren::Wren( Wren&& other )
:   vm_( other.vm_ ),
    refCount_( other.refCount_ ) {
    other.vm_ = nullptr;
    other.refCount_ = nullptr;
}

Wren& Wren::operator=( Wren&& rhs ) {
    release_();
    vm_         = rhs.vm_;
    refCount_   = rhs.refCount_;
    rhs.vm_       = nullptr;
    rhs.refCount_ = nullptr;
    return *this;
}

Wren::~Wren() {
    release_();
}

void Wren::retain_() {
    *refCount_ += 1u;
}

void Wren::release_() {
    if ( refCount_ ) {
        *refCount_ -= 1u;
        if ( *refCount_ == 0u ) {
            // free the VM here
            delete refCount_;
            refCount_ = nullptr;
        }
    }
}

}
