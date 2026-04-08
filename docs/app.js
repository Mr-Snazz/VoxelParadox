const searchInput = document.querySelector("#doc-search");
const searchFeedback = document.querySelector("#search-feedback");
const sections = [...document.querySelectorAll(".doc-section")];
const navLinks = [...document.querySelectorAll("[data-nav]")];
const menuToggle = document.querySelector(".menu-toggle");
const nav = document.querySelector(".section-nav");
const emptyState = document.querySelector("#empty-state");

function normalize(value) {
  return value
    .toLowerCase()
    .normalize("NFD")
    .replace(/[\u0300-\u036f]/g, "");
}

function filterDocs(rawQuery) {
  const query = normalize(rawQuery.trim());
  let visibleCount = 0;

  sections.forEach((section) => {
    const searchable = normalize(
      `${section.id} ${section.dataset.search || ""} ${section.textContent || ""}`
    );
    const matches = query === "" || searchable.includes(query);
    section.classList.toggle("is-hidden", !matches);
    if (matches) {
      visibleCount += 1;
    }
  });

  navLinks.forEach((link) => {
    const targetId = link.getAttribute("href")?.slice(1);
    const target = targetId ? document.getElementById(targetId) : null;
    const visible = !!target && !target.classList.contains("is-hidden");
    link.classList.toggle("is-hidden", !visible);
  });

  if (emptyState) {
    emptyState.classList.toggle("is-hidden", visibleCount !== 0);
  }

  if (searchFeedback) {
    if (query === "") {
      searchFeedback.textContent = "Mostrando todas as secoes.";
    } else if (visibleCount === 0) {
      searchFeedback.textContent = `Nenhuma secao encontrada para "${rawQuery.trim()}".`;
    } else if (visibleCount === 1) {
      searchFeedback.textContent = `1 secao encontrada para "${rawQuery.trim()}".`;
    } else {
      searchFeedback.textContent = `${visibleCount} secoes encontradas para "${rawQuery.trim()}".`;
    }
  }
}

function activateNavLink(id) {
  navLinks.forEach((link) => {
    const active = link.getAttribute("href") === `#${id}`;
    link.classList.toggle("is-active", active);
  });
}

if (searchInput) {
  searchInput.addEventListener("input", (event) => {
    filterDocs(event.target.value);
  });
}

if (menuToggle && nav) {
  menuToggle.addEventListener("click", () => {
    const isOpen = nav.classList.toggle("is-open");
    menuToggle.setAttribute("aria-expanded", String(isOpen));
  });
}

navLinks.forEach((link) => {
  link.addEventListener("click", () => {
    if (nav && nav.classList.contains("is-open")) {
      nav.classList.remove("is-open");
      menuToggle?.setAttribute("aria-expanded", "false");
    }
  });
});

const observer = new IntersectionObserver(
  (entries) => {
    const visibleEntry = entries
      .filter((entry) => entry.isIntersecting)
      .sort((a, b) => b.intersectionRatio - a.intersectionRatio)[0];

    if (visibleEntry?.target?.id) {
      activateNavLink(visibleEntry.target.id);
    }
  },
  {
    rootMargin: "-15% 0px -55% 0px",
    threshold: [0.12, 0.25, 0.4, 0.6]
  }
);

sections.forEach((section) => observer.observe(section));
filterDocs("");
