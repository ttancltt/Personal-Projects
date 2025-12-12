const menuIcon = document.querySelector('.menu-icon');
  const menu = document.querySelector('.mobile-menu');

  menuIcon.addEventListener('click', () => {
    menu.style.display = menu.style.display === 'block' ? 'none' : 'block';
  });
