# The module the binding tests declare functions from.
calls = 0


def total(items):
    return sum(item["price"] * item["quantity"] for item in items)


def count():
    global calls
    calls += 1
    return calls


def greet(name, excited):
    return "hello " + name + ("!" if excited else "")


def maybe(n):
    return n if n > 0 else None


def orElse(value, fallback):
    return value or fallback


def checksum(data):
    return sum(data) & 0xFF


def squares(n):
    return [i * i for i in range(n)]


def halves(n):
    return tuple(i / 2 for i in range(n))


def words(text):
    return text.split()


def flags():
    return [True, False, True]


def reversed_(data):
    return bytes(reversed(data))


def big():
    return 1 << 62


def huge():
    return 1 << 80


def tooBig():
    return 300


def refuse(reason):
    raise ValueError(reason)


def nothing():
    pass


def summary(item):
    return {"label": "%s x%d" % (item["name"], item["quantity"]), "tags": item["tags"]}


class Receipt:
    def __init__(self, items, note):
        self.label = "%d lines" % len(items)
        self.lines = len(items)
        self.paid = True
        self.total = total(items)
        self.tags = [item["name"] for item in items]
        self.first = items[0]
        self.all = items
        self.byName = {item["name"]: item["price"] for item in items}
        self.note = note


def receipt(items, note):
    return Receipt(items, note)


def receipts(items):
    return [vars_of(Receipt([item], None)) for item in items]


def vars_of(r):
    return {
        "label": r.label,
        "lines": r.lines,
        "paid": r.paid,
        "total": r.total,
        "tags": r.tags,
        "first": r.first,
        "all": r.all,
        "byName": r.byName,
    }


def priceList(items):
    return {item["name"]: item["price"] for item in items}


def index(items):
    return {item["name"]: item for item in items}


def wrongShape():
    return {"label": {}}


notAFunction = 42
