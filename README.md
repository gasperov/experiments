# C++ Dojo: Experiments

Welcome, noble code warrior, to the C++ Dojo! This repository is your training ground, where you will encounter the mysterious art of intrusive weak references, among other C++ challenges. 

## The Dojo's Training Tools

### RefCountBase

Think of this as your basic training dummy. It's a base class for objects that can be reference counted. It'll let you increment and decrement the reference count, check if it's still standing, and even tell you its current reference count.

### RefCount

This is your trusty katana, a template class for managing references to objects derived from `RefCountBase`. It'll help you increment and decrement the reference count, reset the reference, and get the current reference count. 

**Warning:** Your katana is sharp, but it's not perfect. Changes to `RefCount` are not thread-safe. If you're going to face multiple opponents (threads), you'll need to bring your own armor (synchronization mechanisms).

### WeakReg

This is your secret weapon, a class for managing weak references. It'll let you add an object to the registry, get a strong reference from a weak one, and release a weak reference.

## The Path of the Code Warrior

This dojo is not just about weak references. It's a place for multiple experiments, a playground for C++ hackers. So, keep your eyes open for new challenges and your mind ready for new learnings.

## Join the Dojo

To join the dojo, simply clone this repository and start training. Remember, the path of the code warrior is not an easy one, but with perseverance and a sense of humor, you'll master the art of C++.

## Acknowledgments

* A special thanks to GitHub Copilot for assisting in the creation of this README. Your AI-powered insights were invaluable, noble code companion!

## License

This dojo follows the way of the open source. It's licensed under the MIT License. See the LICENSE file for details.