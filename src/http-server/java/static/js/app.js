// Create this as a new file: static/js/app.js

document.addEventListener('DOMContentLoaded', function() {
    function refreshContacts() {
        fetch('/api/contacts')
            .then(response => response.json())
            .then(data => {
                const contactsContainer = document.getElementById('contacts-container');
                if (!contactsContainer) return;
                
                if (contactsContainer.children.length === 0) {
                    updateContactList(data, contactsContainer);
                    return;
                }
                
                const currentContacts = {};
                Array.from(contactsContainer.querySelectorAll('.contact')).forEach(contact => {
                    const name = contact.querySelector('h3').textContent;
                    currentContacts[name] = contact;
                });
                
                const contactsChanged = 
                    Object.keys(data).length !== Object.keys(currentContacts).length ||
                    Object.keys(data).some(name => !currentContacts[name]);
                    
                if (contactsChanged) {
                    updateContactList(data, contactsContainer);
                }
            })
            .catch(error => console.error('Error fetching contacts:', error));
    }

    function updateContactList(data, container) {
        container.innerHTML = '';
        
        if (Object.keys(data).length === 0) {
            container.innerHTML = '<p class="empty-message">No contacts yet. <a href="/add">Add one!</a></p>';
            return;
        }
        
        for (const [name, contact] of Object.entries(data)) {
            const contactDiv = document.createElement('div');
            contactDiv.className = 'contact';
            
            if (contact.has_photo) {
                const img = document.createElement('img');
                img.src = `/photo/${encodeURIComponent(name)}`;
                img.alt = name;
                img.loading = 'lazy';
                
                img.className = `contact-photo-${name.replace(/\s+/g, '-')}`;
                
                img.onerror = function() {
                    if (!this.hasAttribute('data-retry')) {
                        this.setAttribute('data-retry', 'true');
                        setTimeout(() => {
                            this.src = `/photo/${encodeURIComponent(name)}?t=${Date.now()}`;
                        }, 500);
                    } else {
                        this.onerror = null;
                        this.src = '/static/images/default.png';
                        console.error(`Failed to load image for contact: ${name}`);
                    }
                };
                contactDiv.appendChild(img);
            }
            
            const infoDiv = document.createElement('div');
            infoDiv.className = 'contact-info';
            
            const nameHeading = document.createElement('h3');
            nameHeading.textContent = name;
            infoDiv.appendChild(nameHeading);
            
            const phoneP = document.createElement('p');
            phoneP.textContent = `Phone: ${contact.phone}`;
            infoDiv.appendChild(phoneP);
            
            const actionsDiv = document.createElement('div');
            actionsDiv.className = 'actions';
            
            const editLink = document.createElement('a');
            editLink.href = `/edit/${encodeURIComponent(name)}`;
            editLink.textContent = 'Edit';
            actionsDiv.appendChild(editLink);
            
            const deleteLink = document.createElement('a');
            deleteLink.href = `/delete/${encodeURIComponent(name)}`;
            deleteLink.textContent = 'Delete';
            actionsDiv.appendChild(deleteLink);
            
            infoDiv.appendChild(actionsDiv);
            contactDiv.appendChild(infoDiv);
            
            const clearDiv = document.createElement('div');
            clearDiv.style.clear = 'both';
            contactDiv.appendChild(clearDiv);
            
            container.appendChild(contactDiv);
        }
    }
    
    setupAjaxForms();
    
    refreshContacts();
    
    setInterval(refreshContacts, 30000);
    
    function setupAjaxForms() {
        const addForm = document.getElementById('add-contact-form');
        if (addForm) {
            addForm.addEventListener('submit', function(e) {
                handleFormSubmit(e, this, '/add', function() {
                    window.location.href = '/';
                });
            });
        }
        
        const editForm = document.getElementById('edit-contact-form');
        if (editForm) {
            editForm.addEventListener('submit', function(e) {
                const name = this.getAttribute('data-name');
                handleFormSubmit(e, this, `/edit/${encodeURIComponent(name)}`, function() {
                    window.location.href = '/';
                });
            });
        }
        
        const deleteForm = document.getElementById('delete-contact-form');
        if (deleteForm) {
            deleteForm.addEventListener('submit', function(e) {
                const name = this.getAttribute('data-name');
                handleFormSubmit(e, this, `/delete/${encodeURIComponent(name)}`, function() {
                    window.location.href = '/';
                });
            });
        }
        
        const searchForm = document.getElementById('search-form');
        const searchInput = document.getElementById('search-input');
        if (searchForm && searchInput) {
            searchForm.addEventListener('submit', function(e) {
                e.preventDefault();
                const query = searchInput.value.trim();
                if (query.length > 0) {
                    fetch(`/api/contacts?q=${encodeURIComponent(query)}`)
                        .then(response => response.json())
                        .then(data => {
                            updateSearchResults(data);
                        })
                        .catch(error => console.error('Error searching contacts:', error));
                }
            });
        }
    }
    
    function updateSearchResults(data) {
        const contactsContainer = document.getElementById('contacts-container');
        if (!contactsContainer) return;
        
        contactsContainer.innerHTML = '';
        
        if (Object.keys(data).length === 0) {
            contactsContainer.innerHTML = '<p class="empty-message">No matching contacts found.</p>';
            return;
        }
        
        for (const [name, contact] of Object.entries(data)) {
            const contactDiv = document.createElement('div');
            contactDiv.className = 'contact';
            
            if (contact.has_photo) {
                const img = document.createElement('img');
                img.src = `/photo/${encodeURIComponent(name)}`;
                img.alt = name;
                contactDiv.appendChild(img);
            }
            
            const infoDiv = document.createElement('div');
            infoDiv.className = 'contact-info';
            
            const nameHeading = document.createElement('h3');
            nameHeading.textContent = name;
            infoDiv.appendChild(nameHeading);
            
            const phoneP = document.createElement('p');
            phoneP.textContent = `Phone: ${contact.phone}`;
            infoDiv.appendChild(phoneP);
            
            const actionsDiv = document.createElement('div');
            actionsDiv.className = 'actions';
            
            const editLink = document.createElement('a');
            editLink.href = `/edit/${encodeURIComponent(name)}`;
            editLink.textContent = 'Edit';
            actionsDiv.appendChild(editLink);
            
            const deleteLink = document.createElement('a');
            deleteLink.href = `/delete/${encodeURIComponent(name)}`;
            deleteLink.textContent = 'Delete';
            actionsDiv.appendChild(deleteLink);
            
            infoDiv.appendChild(actionsDiv);
            contactDiv.appendChild(infoDiv);
            
            const clearDiv = document.createElement('div');
            clearDiv.style.clear = 'both';
            contactDiv.appendChild(clearDiv);
            
            contactsContainer.appendChild(contactDiv);
        }
    }
    
    function handleFormSubmit(event, form, url, successCallback) {
        event.preventDefault();
        
        const formData = new FormData(form);
        
        fetch(url, {
            method: 'POST',
            body: formData
        })
        .then(response => {
            if (response.ok) {
                successCallback();
            } else if (response.status === 400 || response.status === 409) {
                return response.text().then(html => {
                    const tempDiv = document.createElement('div');
                    tempDiv.innerHTML = html;
                    const errorElem = tempDiv.querySelector('.error');
                    if (errorElem) {
                        const existingError = form.querySelector('.error');
                        if (existingError) {
                            existingError.textContent = errorElem.textContent;
                        } else {
                            const newError = document.createElement('p');
                            newError.className = 'error';
                            newError.textContent = errorElem.textContent;
                            form.prepend(newError);
                        }
                    }
                });
            } else {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
        })
        .catch(error => {
            console.error('Error submitting form:', error);
            alert('An error occurred while submitting the form. Please try again.');
        });
    }
});
