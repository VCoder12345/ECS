import argparse
import json
import sys


def load_benchmarks(path):
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    return {
        b["name"]: b
        for b in data["benchmarks"]
        if b.get("run_type") == "iteration"
    }


def percent_change(old, new):
    if old == 0:
        return float("inf") if new != 0 else 0.0
    return (new - old) / old * 100.0


def format_change(old, new, lower_is_better=True):
    change = percent_change(old, new)

    if change == float("inf"):
        return "n/a"

    # For time:
    #   negative = faster
    #   positive = slower
    #
    # For throughput:
    #   positive = faster
    #   negative = slower
    if lower_is_better:
        improved = change < 0
    else:
        improved = change > 0

    if abs(change) < 1.0:
        marker = "="
    elif improved:
        marker = "+"
    else:
        marker = "-"

    return f"{marker} {abs(change):6.2f}%"


def main():
    parser = argparse.ArgumentParser(
        description="Compare two Google Benchmark JSON files."
    )
    parser.add_argument("old", help="Old/baseline benchmark JSON")
    parser.add_argument("new", help="New benchmark JSON")
    parser.add_argument(
        "--sort",
        choices=["name", "change", "regression"],
        default="name",
        help="Sort order (default: name)",
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=0.0,
        help="Only report changes >= this percentage (default: 0)",
    )
    args = parser.parse_args()

    old = load_benchmarks(args.old)
    new = load_benchmarks(args.new)

    old_names = set(old)
    new_names = set(new)

    common = old_names & new_names
    added = new_names - old_names
    removed = old_names - new_names

    results = []

    for name in common:
        o = old[name]
        n = new[name]

        old_real = o.get("real_time")
        new_real = n.get("real_time")

        if old_real is None or new_real is None:
            continue

        change = percent_change(old_real, new_real)

        results.append({
            "name": name,
            "old": o,
            "new": n,
            "change": change,
        })

    if args.sort == "change":
        results.sort(key=lambda x: x["change"], reverse=True)
    elif args.sort == "regression":
        # Biggest regressions first.
        results.sort(key=lambda x: x["change"], reverse=True)
    else:
        results.sort(key=lambda x: x["name"])

    print()
    print("Google Benchmark comparison")
    print("=" * 105)
    print(f"Old: {args.old}")
    print(f"New: {args.new}")
    print()

    print(
        f"{'Benchmark':<48}"
        f"{'Old (ns)':>14}"
        f"{'New (ns)':>14}"
        f"{'Change':>14}"
        f"{'CPU change':>14}"
    )
    print("-" * 105)

    improvements = 0
    regressions = 0

    for r in results:
        name = r["name"]
        o = r["old"]
        n = r["new"]

        real_change = r["change"]

        # Skip small changes if requested.
        if abs(real_change) < args.threshold:
            continue

        old_real = o["real_time"]
        new_real = n["real_time"]

        old_cpu = o.get("cpu_time")
        new_cpu = n.get("cpu_time")

        cpu_change = (
            percent_change(old_cpu, new_cpu)
            if old_cpu is not None and new_cpu is not None
            else None
        )

        if real_change < -args.threshold:
            improvements += 1
        elif real_change > args.threshold:
            regressions += 1

        cpu_str = (
            f"{cpu_change:+.2f}%"
            if cpu_change is not None
            else "n/a"
        )

        print(
            f"{name:<48}"
            f"{old_real:>14,.1f}"
            f"{new_real:>14,.1f}"
            f"{real_change:>+13.2f}%"
            f"{cpu_str:>14}"
        )

    print("-" * 105)

    print(
        f"\nImproved: {improvements}   "
        f"Regressed: {regressions}"
    )

    if added:
        print("\nAdded benchmarks:")
        for name in sorted(added):
            print(f"  + {name}")

    if removed:
        print("\nRemoved benchmarks:")
        for name in sorted(removed):
            print(f"  - {name}")

    if not common:
        print("\nERROR: No matching benchmark names found.")
        sys.exit(1)


if __name__ == "__main__":
    main()
