// The module the binding tests declare functions from.
let calls = 0

export function total(items) {
    return items.reduce((sum, item) => sum + item.price * item.quantity, 0)
}

export function count() {
    calls++
    return calls
}

export const greet = (name, excited) => `hello ${name}${excited ? '!' : ''}`

export function maybe(n) {
    return n > 0 ? n : null
}

export function checksum(bytes) {
    let sum = 0
    for (const b of bytes) sum = (sum + b) & 0xff
    bytes[0] = sum
    return sum
}

export function scale(samples, factor) {
    for (let i = 0; i < samples.length; i++) samples[i] *= factor
    return samples.constructor.name
}

export const squares = (n) => Array.from({ length: n }, (_, i) => i * i)

export const halves = (n) => new Float32Array(n).map((_, i) => i / 2)

export const words = (text) => text.split(' ')

export const flags = () => [true, false, true]

export const reversed = (bytes) => new Uint8Array(bytes).reverse()

export const big = () => 2n ** 62n

export const tooBig = () => 300

export async function later(n) {
    await null
    return n * 2
}

export function refuse(reason) {
    throw new RangeError(reason)
}

export function nothing() {}

export const summary = (item) => ({ label: `${item.name} x${item.quantity}`, tags: item.tags })

export const orElse = (value, fallback) => value ?? fallback

export const receipt = (items, note) => ({
    label: `${items.length} lines`,
    lines: items.length,
    paid: true,
    total: total(items),
    tags: items.map((item) => item.name),
    first: items[0],
    all: items,
    byName: Object.fromEntries(items.map((item) => [item.name, item.price])),
    note,
})

export const receipts = (items) => items.map((item) => receipt([item], null))

export const priceList = (items) => Object.fromEntries(items.map((item) => [item.name, item.price]))

export const index = (items) => Object.fromEntries(items.map((item) => [item.name, item]))

export const wrongShape = () => ({ label: 5 })

export const notAFunction = 42
