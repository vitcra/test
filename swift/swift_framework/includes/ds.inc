<?php

/**
 * @file
 * DS component callbacks.
 */

/**
 * Get the translation links for the current topic node.
 * most probably used to render the language switcher
 *
 */
function _swift_framework_get_content_translation_links($topic) {

  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();
  $currentContent = $currentPage->getContent();

  $topic_translations = translation_node_get_translations($topic->tnid);

  $childContext = FALSE;
  if ($currentPage->hasTopicContext() && $currentPage->isTopicChildPage()) {
    $childContext = TRUE;
    $child_translations = translation_node_get_translations($currentContent->tnid);
  }

  $default_language = language_default('language');

  $links = array();
  foreach (i18n_node_language_list($topic) as $langcode => $langname) {
    $index = 'translation_' . $langcode;
    if ($langcode == $topic->language) {
      $link = array(
        '#markup' => $langcode == 'zh-hans' ? 'zh' : $langcode,
      );
      $links[$index] = render($link);
    }
    elseif (isset($topic_translations[$langcode]) && $topic_translations[$langcode]->status && !isset($links[$index])) {
      $language = i18n_language_object($langcode);
      $alias_language = $language->language;

      if ($childContext) {
        if (!empty($child_translations[$langcode])) {
          $nid = $child_translations[$langcode]->nid;
          $title = $child_translations[$langcode]->title;
        }
        elseif (!empty($currentContent->tnid)) {
          $nid = $currentContent->tnid;
          $title = $currentContent->title;
          // implicit assumption here is that default language is English
          // we also know it cannot be LANGUAGE_NONE, since
          // there is no point for a language switcher then
          $alias_language = $default_language;
        }
        else {
          $nid = $currentContent->nid;
          $title = $currentContent->title;
        }
      }
      else {
        $nid = $topic_translations[$langcode]->nid;
        $title = $topic_translations[$langcode]->title;
      }

      $link = array(
        '#type' => 'link',
        '#href' => drupal_get_path_alias('node/'. $nid, $alias_language),
        '#title' => $language->language == 'zh-hans' ? 'zh' : $language->language,
        '#language' => $language,
        '#attributes' => array(
          'title' => $title,
          'class' => array('translation-link'),
        ),
      );

      if ($childContext && empty($child_translations[$langcode])) {
        $link['#options']['query']['tl'] = $langcode;
      }

      $links[$index] = render($link);
    }
  }
  if (count($links) > 1) {
    return $links;
  } else {
    return array();
  }

}

function _swift_framework_ds_available_translations($field) {
  // Store the node, this should be a topic type.
  $node = $field['entity'];

  if (!empty($node->tnid)) {
    $links = _swift_framework_get_content_translation_links($node);
  }

  if (!empty($links)) {
    $list = theme('item_list', array('items' => $links));
    return $list;
  }
}

function _swift_framework_ds_no_translation_warning($field) {
  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if ($currentPage->hasTopicContext()) {
    $node = $field['entity'];
    $topic = $currentPage->getCurrentTopic();
    if ($node->language !== LANGUAGE_NONE && $topic->language !== LANGUAGE_NONE && $node->language != $topic->language) {

      $text = t('This content is not yet available in the language selected.', array(), array('langcode' => $topic->language));

      return '<div class="group-message"><div class="field-group-wrapper">'. $text .'</div></div>';
    }
  }
}


function _swift_framework_ds_topic_right_col($field) {
  // first check if the node has its own non-empty right col
  $node = $field['entity'];
  if (!empty($node->field_collection_right_col[LANGUAGE_NONE][0])) {
    return;
  }


  // if not display the right col from the topic if it exists in the right_col view mode
  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if ($currentPage->hasTopicContext()) {
    $topic = $currentPage->getCurrentTopic();
    return _sdc_render_node($topic, 'right_col');
  }
}

function _swift_framework_ds_topic_footer($field) {
  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if ($currentPage->hasTopicContext()) {
    $topic = $currentPage->getCurrentTopic();
    return _sdc_render_node($topic, 'footer');
  }
}

function _swift_framework_ds_resource_translated($field){
  if (!empty($field['entity']->tnid)) {
    $nid = $field['entity']->nid;
    $tnid = $field['entity']->tnid;

    $languages = language_list();

    $translated = translation_node_get_translations($tnid);
    $links = array();
    foreach ($translated as $t) {
      if ($nid != $t->nid) {
        $node = node_load($t->nid);
        if (user_is_anonymous() && !empty($node->field_domain_of_interest[LANGUAGE_NONE][0]['tid'])) {
          // Use the Salesforce resource download form.
          module_load_include('inc', 'swift_rc', 'includes/swift_rc.helpers');
          $uri = _swift_rc_salesforce_download_form_uri($node);
        }
        else {
          // Regular download link.
          $uri = file_entity_download_uri((object) $node->field_main_asset[LANGUAGE_NONE][0]);
        }
        $links[] = l($languages[$t->language]->name, $uri['path'], $uri['options']);
      }
    }

    if (count($links)) {
      return theme('item_list', array('items' => $links));
    }

  }
}
