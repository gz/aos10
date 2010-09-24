import copy, os

def uniq(lst):
    # This is ineffcient but keeps it in order
    new_lst = []
    for each in lst:
        if each not in new_lst:
            new_lst.append(each)
    return new_lst


class mkstr:
    def __init__(self, str):
        self.str = str

    def __call__(self, targets, source, env):
        return self.str

    def __str__(self):
        return self.str

def contains_any(needles, haystack):
    """ Returns True if any of the needles are in haystack

    >>> contains_any([1, 2, 3], [50, 37, 12, 1])
    True
    >>> contains_any([1, 2, 3], [50, 37, 12])
    False
    """
    for needle in needles:
        if needle in haystack:
            return True
    return False

def list2str(list):
    """Given a list return a string"""
    return "[" + ", ".join([str(x) for x in list]) + "]"

def make_global(obj, name=None):
    """ Inserts an object into the global namespace. By default
    the name is determined by the objects __name__ attribute. Alternatively
    a name can be specified.

    >>> make_global(1, "One")
    >>> One
    1
    """
    if name:
        globals()[name] = obj
    else:
        globals()[obj.__name__] = obj

class IdentityMapping:
    """ This is an object that can be used to generate an identity mapping.
    That is it is an object that has the same interface as a normal dictionary,
    but every lookup returns the key itself. E.g:

    >>> identity_mapping = IdentityMapping()
    >>> identity_mapping[1]
    1
    >>> identity_mapping["foo"]
    'foo'
    """
    def __getitem__(self, key):
        return key

identity_mapping = IdentityMapping()

class OrderedDict:
    """A very simple insertion ordered dictionary so we can gets
    out keys in insertion order. This is used because we need to link
    against libraries in the order in which the are added"""
    
    def __init__(self):
        self.dict = {}
        self.insert_order = []

    def __setitem__(self, key, value):
        self.dict[key] = value
        self.insert_order.append(key)

    def __getitem__(self, key):
        return self.dict[key]

    def keys(self):
        return self.insert_order

    def __copy__(self):
        new = OrderedDict()
        new.dict = copy.copy(self.dict)
        new.insert_order = copy.copy(self.insert_order)
        return new

    def items(self):
        return [self.dict[key] for key in self.keys()]

class ListFunction:
    def __init__(self, fn):
        self.fn = fn

    def __getitem__(self, idx):
        return self.fn()[idx]

def markup(template, output, environment):
	"""
	This function reads a template. The template format is:
	{{python code:}} execute "python code"
	{{?python code}}: evaluate "python code" and replace the whole thing with the result.
	{{
	python code
	...
	}}: execute "python code". 
	"""		
	environment['out'] = output
	
	window_start = 0
	window_end = 0
	eof = len(template)

	STATE_NORMAL = 0
	STATE_EXECCODE = 1
	STATE_EVALCODE = 2
	STATE_LOOP = 3
	STATE_IF = 4

	state = STATE_NORMAL

	while window_start < eof:
		if state == STATE_NORMAL:
			window_end = template.find('{{', window_start)
			if window_end == -1:
				window_end = eof
			output.write(template[window_start:window_end])
			window_start = window_end
			if template[window_start:window_start + 9] == '{{ABORT}}':
				return False
			if template[window_start:window_start + 4] == '{{if':
				state = STATE_IF
				window_start += 4
			elif template[window_start:window_start + 3] == '{{?':
				state = STATE_EVALCODE
				window_start += 3
			elif template[window_start:window_start + 3] == '{{*':
				state = STATE_LOOP
				window_start += 3
			elif template[window_start:window_start + 2] == '{{':
				state = STATE_EXECCODE
				window_start += 2
		elif state == STATE_EVALCODE:
			window_end = template.find('}}', window_start)
			if window_end != -1:
				code = template[window_start:window_end]
				output.write(str(eval(code, environment)))
				window_start = window_end + 2
			state = STATE_NORMAL
		elif state == STATE_EXECCODE:
			window_end = template.find('}}', window_start)
			if window_end != -1:
				code = template[window_start:window_end]
				exec code in environment
				window_start = window_end + 2
			state = STATE_NORMAL
		elif state == STATE_IF:
			window_end = template.find('fi}}', window_start)
			ifcode_end = template.find(':', window_start)
			code = template[window_start:ifcode_end]
			if eval(code, environment):
				result = markup(template[ifcode_end+1:window_end], output, environment)
				if result is False:
					return False
			window_start = window_end + 4
			state = STATE_NORMAL
		elif state == STATE_LOOP:
			window_end = template.find('*}}', window_start)
			loopcode = template[window_start:window_end]
			for item in environment['LOOP']:
				environment['LOOPITEM'] = item
				result = markup(loopcode, output, environment)
				if result is False:
					return False
			window_start = window_end + 3
			state = STATE_NORMAL
	return True

# Remove pyc files from a directory
def pyc_clean(dir):
    def rmglob(arg, top, names):
        rmlist = [ top + os.path.sep + x for x in names if x.endswith(arg)]
        for x in rmlist:
            print "Removing", x
            os.remove(x)
    os.path.walk(dir, rmglob, ".pyc")

class LibraryNotFound(Exception):
    pass

#   Temporarily change ALIGNMENT to 0x100000 from 0x10000.  This
#   results in each program residing on separate 1MB virtual memory
#   sections.  Thus, two different PDs should never contend with
#   the same 1MB section causing extraneous page faults.
#   OK, we changed it back to 64KB alignment because this gives us
#   more contiguous virtual memory for the AMSS heap
ALIGNMENT=0x10000

def align(val):
    ovr = val % ALIGNMENT
    if (ovr):
        val = val + ALIGNMENT - ovr
    return val

def align_down(val):
    return val & ~(ALIGNMENT - 1)

