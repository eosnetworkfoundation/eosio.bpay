import { Asset, Name } from '@wharfkit/antelope'
import { describe, expect, test } from 'bun:test'
import { Blockchain, expectToThrow } from '@eosnetwork/vert'

// Vert EOS VM
const blockchain = new Blockchain()

// 21 producers
const alice = "bp.alice"
const bob = "bp.bob"
const charly = "bp.charly"
const producers = [
    alice,
    bob,
    charly,
    "bp.d",
    "bp.e",
    "bp.f",
    "bp.g",
    "bp.h",
    "bp.i",
    "bp.j",
    "bp.k",
    "bp.l",
    "bp.m",
    "bp.n",
    "bp.o",
    "bp.p",
    "bp.q",
    "bp.r",
    "bp.s",
    "bp.t",
    "bp.u",
]
const standby = "bp.standby"
const inactive = "bp.inactive"
blockchain.createAccounts(...producers, standby, inactive)

const bpay_contract = 'eosio.bpay'
const contracts = {
    bpay: blockchain.createContract(bpay_contract, bpay_contract, true),
    token: blockchain.createContract('eosio.token', 'external/eosio.token/eosio.token', true),
    system: blockchain.createContract('eosio', 'external/eosio.system/eosio', true),
    fake: {
        token: blockchain.createContract('fake.token', 'external/eosio.token/eosio.token', true),
    },
}

function getProducers(owner: string) {
    const scope = Name.from("eosio").value.value
    const primary_key = Name.from(owner).value.value
    return contracts.system.tables
        .producers(scope)
        .getTableRow(primary_key)
}

function getRewards(owner: string) {
    const scope = Name.from(bpay_contract).value.value
    const primary_key = Name.from(owner).value.value
    const row = contracts.bpay.tables
        .rewards(scope)
        .getTableRow(primary_key)
    if (!row) return 0;
    return Asset.from(row.quantity).units.toNumber()
}

function getTokenBalance(account: string, symcode: string) {
    const scope = Name.from(account).value.value
    const primary_key = Asset.SymbolCode.from(symcode).value.value
    const row = contracts.token.tables
        .accounts(scope)
        .getTableRow(primary_key)
    if (!row) return 0;
    return Asset.from(row.balance).units.toNumber()
}

describe(bpay_contract, () => {
    test('eosio::setproducers', async () => {
        let votes = 100.0;
        for ( const producer of producers ) {
            await contracts.system.actions.setproducer([producer, votes, true]).send()
            votes -= 1.0;
        }
        await contracts.system.actions.setproducer([standby, 1, true]).send() // lowest vote
        await contracts.system.actions.setproducer([inactive, 200.0, false]).send() // highest vote, but inactive
    })

    test('eosio.token::issue::EOS', async () => {
        const supply = `1000000000.0000 EOS`
        await contracts.token.actions.create(['eosio.token', supply]).send()
        await contracts.token.actions.issue(['eosio.token', supply, '']).send()
    })

    test("eosio.bpay::transfer", async () => {
        await contracts.token.actions.transfer(['eosio.token', bpay_contract, '2100.0000 EOS', '']).send();
        const before = {
            bpay: {
                balance: getTokenBalance(bpay_contract, 'EOS'),
            },
            alice: {
                balance: getTokenBalance(alice, 'EOS'),
            },
            bob: {
                balance: getTokenBalance(bob, 'EOS'),
            },

        }
        await contracts.bpay.actions.claimrewards([alice]).send(alice);

        const after = {
            bpay: {
                balance: getTokenBalance(bpay_contract, 'EOS'),
            },
            alice: {
                balance: getTokenBalance(alice, 'EOS'),
            },
            bob: {
                balance: getTokenBalance(bob, 'EOS'),
            },
        }

        // EOS
        expect(after.bpay.balance - before.bpay.balance).toBe(-1000000)
        expect(after.alice.balance - before.alice.balance).toBe(1000000)
        expect(after.bob.balance - before.bob.balance).toBe(0)
    });

    test('eosio.bpay::claimrewards::error - no rewards to claim', async () => {
        const action = contracts.bpay.actions.claimrewards([alice]).send(alice);
        await expectToThrow(action, 'eosio_assert: no rewards to claim');
    })

    test.skip('eosio.bpay::claimrewards::error - not eligible to claim block rewards', async () => {
        for ( const owner of [inactive, standby] ) {
            const action = contracts.bpay.actions.claimrewards([owner]).send(owner);
            await expectToThrow(action, 'eosio_assert: not eligible to claim producer block pay');
        }
    })

})