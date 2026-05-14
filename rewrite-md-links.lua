-- rewrite-md-links.lua — Pandoc Lua filter: rewrite local .md links to .html
-- Used by mdview to make inter-file markdown links work in the browser.

function Link(el)
  -- Skip external URLs
  if el.target:match("^https?://") or el.target:match("^mailto:") then
    return el
  end
  -- Rewrite .md#fragment → .html#fragment
  el.target = el.target:gsub("%.md(#)", ".html%1")
  -- Rewrite .md (no fragment) → .html
  el.target = el.target:gsub("%.md$", ".html")
  return el
end
