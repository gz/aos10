#!/usr/bin/python
#
# Authour: Philip O'Sullivan <philip.osullivan@nicta.com.au>
# Date   : Wed 22 Mar 2006
#

"""
This module is defines data structures used to describe all the possible 
configurations of variables of the form 'name' = 'value'.

The L{Variable}, L{And} and L{Or} classes are used to construct an And/Or tree which
describes all the possible combinations for valid arguements.  All the
combinations are accessed throught the in-built iterators of the class.

The following is an example:

>>> from config_gen import Variable, And, Or
>>>
>>> fruit = Variable('fruit', ['apple', 'bananna'])
>>> colour = Variable('colour', ['red', 'blue', 'green'])
>>> widget = Variable('widget', ['foo', 'bar'])
>>> 
>>> tree = And([Or([fruit, colour]), widget])
>>> 
>>> for lst in tree:
>>>        print ' '.join(['%s=%s' % (x[0], x[1]) for x in lst])
fruit=apple widget=foo
fruit=apple widget=bar
fruit=bananna widget=foo
fruit=bananna widget=bar
colour=red widget=foo
colour=red widget=bar
colour=blue widget=foo
colour=blue widget=bar
colour=green widget=foo
colour=green widget=bar
"""

__revision__ = '1.0'

class Node:
        """
        Abstract base class for classes implementing a And/Or tree.
        """
        def __init__(self, children):
                """
                Initialise the children for a subclass of L{Node}.

                @type children: list[L{Node}|L{Variable}]
                @param children: The node's children in the And/Or tree
                """
                assert type(children) == type([])
                self.children = children

        def tail(self):
                """
                Return a Node built from the tail of the children (i.e. children[1:]).

                @rtype: Node
                @return: Node of the tail of the Node's children
                """
                raise NotImplementedError


class Variable(Node):
        """
        This class is used to represent an name=val arguement that is passed
        to scons with all the possible values of val.
        """
        def __init__(self, name, children):
                """
                Initialise a new instance of Variable

                @type name: string
                @param name: the name of the variable

                @type children: list[string]
                @param children: list of all the values that this variable can take

                @rtype: Variable
                @return: The initialised instance of Variable
                """
                self.name = name
                Node.__init__(self, children)

        def __iter__(self):
                def var_gen(args):
                        """
                        Generator for the class that returns (name,val) 
                        tuples for each val in children.
                        """
                        if args == []:
                                return
                        for arg in args:
                                yield (self.name, arg)
                        return
                return var_gen(self.children)


class And(Node):
        """
        Instances of this class are nodes in a tree that return a list with 
        a yielded value of their children on each iteration.
        """
        def __init__(self, children):
                Node.__init__(self, children)

        def __iter__(self):
                def and_gen(args):
                        """
                        Generator for the class that returns a list with
                        something from each of its children.
                        """
                        if args.children == []:
                                yield args.children
                                return
                        for i in args.children[0]:
                                for tmp in and_gen(args.tail()):
                                        yield flatten([i] + tmp)
                        return
                return and_gen(self)

        def tail(self):
                """
                Return a L{And} built from the tail of the children (i.e. children[1:]).

                @rtype: L{Node}
                @return: L{And} built from the tail of the node's children
                """
                return And(self.children[1:])


class Or(Node):
        """
        Instances of this class are nodes in a tree that return a single 
        value, starting from the first child and then working through all their
        children in order, on each iteration.
        """
        def __init__(self, children):
                Node.__init__(self, children)

        def __iter__(self):
                def or_gen(args):
                        """A generator for the class that returns one thing,
                        starting from the first child and moving through all
                        the children"""
                        if args.children == []:
                                return
                        for i in args.children[0]:
                                yield flatten([i])
                        for i in or_gen(args.tail()):
                                yield i
                        return 
                return or_gen(self)

        def tail(self):
                """
                Return a Node built from the tail of the children (i.e. children[1:]).

                @rtype: Node
                @return: Node of the tail of the Node's children
                """
                return Or(self.children[1:])


def flatten(lst):
        """
        Flatten a list containing lists into a single level

        @type lst: list
        @param lst: list that may contain other lists or tuples

        @rtype: list
        @return: list that does not contain lists or tuples
        """
        if type(lst) != type([]): 
                return [lst]
        if lst == []: 
                return lst
        return flatten(lst[0]) + flatten(lst[1:])
