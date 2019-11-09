const setupAutocomplete = (input) => {
  let currentFocus;
  input.setAttribute('valid-input', false);
  input.addEventListener("input", async function(e) {
    const val = this.value;
    const arr = val ? await autocomplete(val) : [];

    closeAllLists();
    input.setAttribute('valid-input', false);
    if (!val) return false;

    currentFocus = -1;
    const a = document.createElement('div');
    a.setAttribute('id', `${this.id}-autocomplete-list`);
    a.setAttribute('class', 'autocomplete-items');
    this.parentNode.appendChild(a);

    for (i = 0; i < arr.length; i++) {
      if (val === arr[i]) input.setAttribute('valid-input', true);
      const b = document.createElement('div');
      b.innerHTML = arr[i];
      b.innerHTML += `<input type='hidden' value='${arr[i]}'>`;
      b.addEventListener('click', function(e) {
        console.log('click');
        input.value = this.getElementsByTagName('input')[0].value;
        input.setAttribute('valid-input', true);
        closeAllLists();
      });
      a.appendChild(b);
    }
  });

  input.addEventListener('keydown', function(e) {
    let x = document.getElementById(`${this.id}-autocomplete-list`);
    if (x) x = x.getElementsByTagName('div');
    if (e.keyCode == 40) { // down
      currentFocus++;
      addActive(x);
    } else if (e.keyCode == 38) { // up
      currentFocus--;
      addActive(x);
    } else if (e.keyCode == 13) { // enter
      e.preventDefault();
      if (currentFocus > -1) {
        if (x) x[currentFocus].click();
      }
    }
  });

  const addActive = (elem) => {
    if (!elem) return false;
    removeActive(elem);
    if (currentFocus >= elem.length) currentFocus = 0;
    if (currentFocus < 0) currentFocus = (elem.length - 1);
    elem[currentFocus].classList.add('autocomplete-active');
  }

  const removeActive = (elem) => {
    for (let i = 0; i < elem.length; i++) {
      elem[i].classList.remove('autocomplete-active');
    }
  }

  const closeAllLists = (elem) => {
    const items = document.getElementsByClassName('autocomplete-items');
    for (let i = 0; i < items.length; i++) {
      if (elem != items[i] && elem != input) {
        items[i].parentNode.removeChild(items[i]);
      }
    }
  }

  document.addEventListener('click', ({ target }) => {
    closeAllLists(target);
  });

  input.addEventListener('blur', (e) => {
    setTimeout(closeAllLists, 250);
  });
} 

setupAutocomplete(document.getElementById("articleSearch1"));
setupAutocomplete(document.getElementById("articleSearch2"));
