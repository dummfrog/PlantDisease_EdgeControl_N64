from __future__ import annotations

import argparse
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description="Browser E2E smoke test for Dashboard.")
    parser.add_argument("--url", default="http://127.0.0.1:8001")
    parser.add_argument("--screenshot", default=str(PROJECT_ROOT / "dashboard_e2e_verify.png"))
    args = parser.parse_args()

    try:
        from playwright.sync_api import sync_playwright
    except ImportError:
        print("playwright is not installed. Run: python -m pip install playwright && python -m playwright install chromium")
        return 2

    console_errors: list[str] = []
    failed_requests: list[str] = []

    with sync_playwright() as playwright:
        browser = playwright.chromium.launch(headless=True)
        page = browser.new_page(viewport={"width": 1440, "height": 1000})
        page.on("console", lambda msg: console_errors.append(msg.text) if msg.type == "error" else None)
        page.on("requestfailed", lambda req: failed_requests.append(req.url))
        page.goto(args.url, wait_until="networkidle", timeout=30000)
        title = page.title()
        if "Dashboard" not in title and "智慧农业" not in title:
            print(f"unexpected title: {title}")
            return 1
        page.locator("#historyBody").wait_for(timeout=10000)
        if page.locator("#diseaseChart").count() == 0:
            print("disease chart canvas not found")
            return 1
        if page.locator("#historyBody tr").count() == 0:
            print("history table has no rows")
            return 1
        page.screenshot(path=args.screenshot, full_page=True)
        browser.close()

    if console_errors or failed_requests:
        print({"console_errors": console_errors, "failed_requests": failed_requests})
        return 1
    print(f"Dashboard E2E passed. Screenshot: {args.screenshot}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
